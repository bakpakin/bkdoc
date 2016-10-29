# This is a sample Makefile for building a large number
# of bkdoc documents in a directory to HTML. This setup works particlularly
# well for things like notes. It relies on the bkd executable being in your path.

INPUTS := $(wildcard ./**/*.bkd)
OUTPUTS := $(patsubst ./%.bkd,./output/%.html,$(INPUTS))

all: $(OUTPUTS)

./output/%.html : ./%.bkd
	@echo "Building $@"
	bkd < $< > $@
