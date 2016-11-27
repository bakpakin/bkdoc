# BKDoc
# Copyright Calvin Rose

CFLAGS=-std=c99 -Wall -Wextra -O4 -g -I include -I src -I cli
TARGET=bkd
PREFIX=/usr/local

# C sources
SOURCES=src/bkd_utf8.c src/bkd_string.c src/bkd_html.c src/bkd_parse.c src/bkd_util.c cli/main.c
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

# Test fixtures
FIXTURES_SOURCE=$(wildcard tests/fixtures/*.bkd)
FIXTURES=$(patsubst %.bkd,%.html,$(FIXTURES_SOURCE))
FIXTURES_TEMP=$(patsubst %.bkd,%.html.tmp,$(FIXTURES_SOURCE))
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

%.html : %.bkd $(TARGET)
	./$(TARGET) -s < $< > $@

# Generate things to test
%.html.tmp : %.bkd $(TARGET)
	@./$(TARGET) -s < $< > $@

# Run test on fixture
%.target : %.html.tmp
	@echo "Testing $<..."
	@diff $< $(patsubst %.html.tmp,%.html,$<)

# Generate test fixtures. Only run to update the tests.
# If there is a bug in the output, this will overwrite
# the coorect output with the incorrect output. Don't use
# this very often.
fixtures: $(FIXTURES)

test: $(FIXTURES_TEMP) $(FIXTURES_TARGET)

.PHONY: clean install test fixtures
