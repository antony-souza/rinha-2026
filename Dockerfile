FROM alpine:3.20 AS compile

RUN apk add --no-cache build-base linux-headers
WORKDIR /app
COPY Makefile .
COPY src ./src
RUN make all

FROM compile AS index

COPY resources/references.json.gz ./resources/references.json.gz
RUN make index

FROM alpine:3.20

WORKDIR /app
COPY --from=index /app/build/rinha-api /app/rinha-api
COPY --from=index /app/build/rinha-lb /app/rinha-lb
COPY --from=index /app/build/references.idx /app/resources/references.idx

ENV PORT=8080
ENV REFERENCES_PATH=/app/resources/references.idx

EXPOSE 8080
CMD ["/app/rinha-api"]
