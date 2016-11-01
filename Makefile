# BKDoc
# Copyright Calvin Rose

CFLAGS=-std=c99 -Wall -Wextra -O4 -s -I include -I src -I cli
TARGET=bkd
PREFIX=/usr/local

# C sources
SOURCES=src/bkd_utf8.c src/bkd_hash.c src/bkd_string.c src/bkd_html.c src/bkd_parse.c src/bkd_util.c cli/main.c
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

# Test fixtures
FIXTURES_SOURCE=$(wildcard tests/fixtures/*.bkd)
FIXTURES=$(patsubst %.bkd,%.html,$(FIXTURES_SOURCE))
FIXTURES_TEMP=$(patsubst %.bkd,%.html.unit,$(FIXTURES_SOURCE))
FIXTURES_TARGET=$(patsubst %.bkd,%.target,$(FIXTURES_SOURCE))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

install: $(TARGET)
	cp $(TARGET) $(PREFIX)/bin

clean:
	rm $(TARGET) || true
	rm $(OBJECTS) || true
	rm $(FIXTURES_TEMP) || true

# Generate test fixtures. Only run to update the tests.
%.html : %.bkd $(TARGET)
	@echo "WARNING: Generating fixture $@..."
	@./$(TARGET) < $< > $@

# Generate things to test
%.html.unit : %.bkd $(TARGET)
	@./$(TARGET) < $< > $@

# Run test on fixture
%.target : %.html %.html.unit
	@echo "Testing $<..."
	@diff $^

fixtures: $(FIXTURES)

test: $(FIXTURES_TARGET)

.PHONY: clean install test fixtures
