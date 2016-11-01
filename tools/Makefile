# This is a sample Makefile for building a large number
# of bkdoc documents in a directory to HTML. This setup works particularly
# well for things like notes. It relies on the bkd executable being in your path.

INPUTS := $(wildcard ./**/*.bkd)
OUTPUTS := $(patsubst ./%.bkd,./%.html,$(INPUTS))

all: $(OUTPUTS)

./%.html : ./%.bkd
	@echo "Building $@"
	bkd < $< > $@
