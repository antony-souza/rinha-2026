#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#ifndef SO_REUSEPORT
#define SO_REUSEPORT 15
#endif

#ifndef TCP_DEFER_ACCEPT
#define TCP_DEFER_ACCEPT 9
#endif

#define BACKEND_COUNT 2

typedef struct {
    const char *path;
    int fd;
} Backend;

static int connect_backend(const char *path) {
    for (;;) {
        int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0) {
            return -1;
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            return fd;
        }

        close(fd);
        struct timespec ts = {.tv_sec = 0, .tv_nsec = 10000000};
        nanosleep(&ts, NULL);
    }
}

static bool send_fd_once(int control_fd, int client_fd) {
    char byte = 1;
    struct iovec iov = {.iov_base = &byte, .iov_len = 1};
    char control[CMSG_SPACE(sizeof(int))];
    memset(control, 0, sizeof(control));

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &client_fd, sizeof(client_fd));

    return sendmsg(control_fd, &msg, MSG_NOSIGNAL) >= 0;
}

static bool send_fd_backend(Backend *backend, int client_fd) {
    if (send_fd_once(backend->fd, client_fd)) {
        return true;
    }

    close(backend->fd);
    backend->fd = connect_backend(backend->path);
    return backend->fd >= 0 && send_fd_once(backend->fd, client_fd);
}

static void tune_client(int fd) {
    int yes = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
    setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &yes, sizeof(yes));
}

static int listen_on(int port) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return -1;
    }

    int yes = 1;
    int defer_accept = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
    setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &defer_accept, sizeof(defer_accept));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    if (listen(fd, 65535) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int main(void) {
    Backend backends[BACKEND_COUNT] = {
        {.path = "/sockets/api1.sock", .fd = -1},
        {.path = "/sockets/api2.sock", .fd = -1},
    };

    for (size_t i = 0; i < BACKEND_COUNT; i++) {
        backends[i].fd = connect_backend(backends[i].path);
        if (backends[i].fd < 0) {
            return 1;
        }
    }

    const char *port_env = getenv("PORT");
    int port = port_env ? atoi(port_env) : 9999;
    int server = listen_on(port);
    if (server < 0) {
        perror("listen");
        return 1;
    }

    fprintf(stderr, "rinha-fd-lb listening on :%d\n", port);
    uint32_t next = 0;

    for (;;) {
        int client = accept4(server, NULL, NULL, SOCK_CLOEXEC);
        if (client < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                struct timespec ts = {.tv_sec = 0, .tv_nsec = 1000000};
                nanosleep(&ts, NULL);
                continue;
            }
            continue;
        }

        tune_client(client);
        uint32_t first = next++ & 1u;
        if (!send_fd_backend(&backends[first], client)) {
            (void)send_fd_backend(&backends[first ^ 1u], client);
        }
        close(client);
    }
}
