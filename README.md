# Rinha de Backend 2026 — C

Implementação da API de detecção de fraude da Rinha de Backend 2026.

## Arquitetura

- Duas instâncias da API em C.
- Load balancer round-robin em C.
- Passagem de descritores de arquivo por Unix sockets.
- Atendimento assíncrono com `epoll`.
- Índice KD-tree pré-processado durante o build.
- Busca vetorial sobre as referências oficiais, sem lookup por payload ou ID de teste.

## Build local

O build gera `references.idx` a partir de `resources/references.json.gz`:

```sh
docker compose -f docker-compose.yml -f docker-compose.local.yml up --build -d
curl http://localhost:9999/ready
```

## Publicar imagem

Troque o nome da imagem se necessário e publique uma versão `linux/amd64`:

```sh
docker buildx build --platform linux/amd64 -t antonybash/rinha-2026:latest --push .
```

O `docker-compose.yml` usa `RINHA_IMAGE` para permitir outro nome sem editar o arquivo:

```sh
RINHA_IMAGE=seu-usuario/sua-imagem:tag docker compose up -d
```

## Endpoints

- `GET /ready`
- `POST /fraud-score`

## Limites

O `docker-compose.yml` soma exatamente:

- `1.00` CPU
- `350 MB` de memória
