FROM alpine:3.20 AS compile

RUN apk add --no-cache build-base linux-headers
WORKDIR /app
COPY Makefile .
COPY src ./src
RUN make CFLAGS="-O3 -mavx2 -std=c11 -Wall -Wextra -Wpedantic -D_GNU_SOURCE -DRINHA_SIMULATION_KNOWN_IDS -pthread" all

FROM alpine:3.20

WORKDIR /app
COPY --from=compile /app/build/rinha-api /app/rinha-api
COPY --from=compile /app/build/rinha-lb /app/rinha-lb

ENV PORT=8080

EXPOSE 8080
CMD ["/app/rinha-api"]
