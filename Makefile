.PHONY: all plugin app test clean

all: app

plugin:
	$(MAKE) -C plugin

app: plugin
	$(MAKE) -C app

# Host unit tests for the shared logic; no PS4 toolchain needed.
test:
	mkdir -p build
	cc -std=c99 -Wall -Wextra -Werror -Icommon -o build/test_common tests/test_common.c common/*.c
	build/test_common

clean:
	$(MAKE) -C plugin clean
	$(MAKE) -C app clean
	rm -rf build
