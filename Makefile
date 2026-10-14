PRODUCT = adam-menu
PLATFORMS += adam

SRC_DIRS = src

# A version, directory, zip file or git URL. The repo for now: the 5.0.0 zip
# names its library wrong.
FUJINET_LIB = https://github.com/FujiNetWIFI/fujinet-lib.git

include mekkogx/toplevel-rules.mk

VERSION ?= $(shell git describe --tags --always --dirty 2>/dev/null | sed 's/^v//')

# The zip a GitHub release carries. MekkoGX's own release target zips a library.
dist: adam
	rm -rf dist
	mkdir -p dist
	zip -j dist/$(PRODUCT)-$(VERSION).zip r2r/adam/$(PRODUCT).ddp README.md Changelog.md
.PHONY: dist
