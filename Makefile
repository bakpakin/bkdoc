# BKDoc
# Copyright Calvin Rose

CFLAGS=-std=c99 -Wall -Wextra -O4 -s -I include -I src -I cli
TARGET=bkd

SOURCES=src/bkd_utf8.c src/bkd_hash.c src/bkd_string.c src/bkd_html.c src/bkd_parse.c src/bkd_util.c cli/main.c
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm $(TARGET) || true
	rm $(OBJECTS) || true

.PHONY: clean
