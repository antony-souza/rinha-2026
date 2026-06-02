CC ?= gcc
CFLAGS ?= -O3 -mavx2 -std=c11 -Wall -Wextra -Wpedantic -D_GNU_SOURCE -pthread
LDFLAGS ?= -lm -pthread

BIN := build/rinha-api
LB := build/rinha-lb
CONVERTER := build/convert-references
INDEX_BUILDER := build/build-index

.PHONY: all clean index

all: $(BIN) $(LB) $(CONVERTER) $(INDEX_BUILDER)

$(BIN): src/main.c src/known_ids.inc
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(LB): src/lb-fd.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(CONVERTER): src/convert-references.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(INDEX_BUILDER): src/build-index.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

index: $(CONVERTER) $(INDEX_BUILDER)
	gzip -dc resources/references.json.gz > build/references.json
	$(CONVERTER) build/references.json build/references.bin
	$(INDEX_BUILDER) build/references.bin build/references.idx

clean:
	rm -rf build
