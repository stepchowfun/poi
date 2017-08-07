# The final build artifacts will be installed in subdirectories of this path.
# For example, binaries will be located in "$PREFIX/bin".
PREFIX := /usr/local

# Determine the build type (release or debug).
BUILD_TYPE := release
ifneq ($(BUILD_TYPE),$(filter $(BUILD_TYPE),release debug))
  $(error BUILD_TYPE must be 'release' or 'debug')
endif
override BUILD_PREFIX := build/$(BUILD_TYPE)

# The headers and sources to compile.
# There is an additional source file not listed here called version.cpp,
# which is built from scripts/version.sh and included in the static library.
override SOURCES := \
  $(shell find src/*.cpp -type f) \
  $(BUILD_PREFIX)/poi/src/version.cpp
override HEADERS := $(shell find include/*.h -type f)
override HEADERS_DIST := \
  $(patsubst include/%,$(BUILD_PREFIX)/dist/include/poi/%,$(HEADERS))
override OBJ := \
  $(BUILD_PREFIX)/poi/obj/$(BUILD_PREFIX)/poi/src/version.o \
  $(patsubst %.cpp,$(BUILD_PREFIX)/poi/obj/%.o,$(SOURCES))

# These targets do not name actual files.
# They are just recipes which may be executed by explicit request.
.PHONY: all lib cli clean lint test install uninstall force

# This flag controls whether $(BUILD_PREFIX)/poi/src/version.cpp needs to be
# rebuilt. Specifically, if scripts/version.sh produces a different output
# than the existing version.cpp file (or the file does not exist), then we
# force a rebuild. We have this flag because scripts/version.sh is not
# idempotent (it depends on the git SHA).
override VERSION_CPP_DEPS := $(shell \
  if test "$$( \
    cksum '$(BUILD_PREFIX)/poi/src/version.cpp' 2> /dev/null \
      | cut -d ' ' -f 1 \
  )" != "$$( \
    ./scripts/version.sh $(BUILD_TYPE) | cksum 2> /dev/null \
      | cut -d ' ' -f 1 \
  )"; then echo 'force'; fi \
)

# This is the default target.
# It builds all artifacts for distribution.
all: lib cli

# This target builds the static library.
lib: $(BUILD_PREFIX)/dist/lib/poi.a $(HEADERS_DIST)

# This target builds the command line interface.
cli: $(BUILD_PREFIX)/dist/bin/poi lib

# This target removes all build artifacts.
clean:
	rm -rf build

# This target runs the linters and static analyzers.
# ShellCheck must be installed for this to work.
lint:
	# Make sure all lines conform to the line length limit.
	scripts/check-line-lengths.sh \
	  .github/* \
	  .travis.yml \
	  Makefile \
	  cli/* \
	  include/* \
	  scripts/* \
	  src/*

	# Check for various style issues related to whitespace.
	SKIP_TABS_CHECK=FALSE scripts/check-whitespace.sh \
	  .github/* \
	  .travis.yml \
	  cli/* \
	  include/* \
	  scripts/* \
	  src/*
	SKIP_TABS_CHECK=TRUE scripts/check-whitespace.sh Makefile

	# Run ShellCheck on any shell scripts.
	shellcheck scripts/*.sh

# This target runs the test suite on the command line interface for the
# specified build type.
test: cli
	# Run all the tests.
	scripts/run-tests.rb test --binary $(BUILD_PREFIX)/dist/bin/poi

# This target installs build artifacts to the $(PREFIX) directory.
# The artifacts will be placed in subdirectories such as $(PREFIX)/bin.
install: all uninstall
	mkdir -p "$(PREFIX)/bin"
	mkdir -p "$(PREFIX)/include/poi"
	mkdir -p "$(PREFIX)/lib"
	cp $(BUILD_PREFIX)/dist/bin/poi "$(PREFIX)/bin/poi"
	cp $(BUILD_PREFIX)/dist/lib/poi.a "$(PREFIX)/lib/poi.a"
	cp $(HEADERS_DIST) "$(PREFIX)/include/poi"

# This target removes all installed artifacts from the $(PREFIX) directory.
uninstall:
	rm -f "$(PREFIX)/bin/poi"
	rm -f "$(PREFIX)/lib/poi.a"
	rm -rf "$(PREFIX)/include/poi"

# This is a phony target that any other target can depend on to force a build.
force:

# This target copies the headers into the $(BUILD_PREFIX)/dist/include/poi
# directory for distribution.
$(HEADERS_DIST): $(HEADERS)
	mkdir -p $$(dirname $@)
	cp $(HEADERS) $(BUILD_PREFIX)/dist/include/poi

# This target generates version.cpp using the scripts/version.sh script.
$(BUILD_PREFIX)/poi/src/version.cpp: $(VERSION_CPP_DEPS)
	mkdir -p $$(dirname $@)
	./scripts/version.sh $(BUILD_TYPE) > \
	  $(BUILD_PREFIX)/poi/src/version.cpp

# This target compiles a source file.
$(BUILD_PREFIX)/poi/obj/%.o: %.cpp $(HEADERS_DIST)
	mkdir -p $$(dirname $@)
	$(CXX) -c $< \
	  -I $(BUILD_PREFIX)/dist/include \
	  $$( \
	    echo $(BUILD_TYPE) | grep -qi 'debug' && \
	    echo '-g -O0' || \
	    echo '-O3' \
	  ) \
	  -std=c++11 \
	  -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter \
	  -o $@

# This target builds the static library.
$(BUILD_PREFIX)/dist/lib/poi.a: $(OBJ)
	mkdir -p $$(dirname $@)
	rm -f $@
	ar rcs $@ $(OBJ)

# This target builds the command line interface.
$(BUILD_PREFIX)/dist/bin/poi: \
  $(BUILD_PREFIX)/dist/lib/poi.a \
  $(BUILD_PREFIX)/poi/obj/cli/main.o \
  $(HEADERS_DIST)
	mkdir -p $$(dirname $@)
	$(CXX) \
	    $(BUILD_PREFIX)/poi/obj/cli/main.o \
	    $(BUILD_PREFIX)/dist/lib/poi.a \
	  -I $(BUILD_PREFIX)/dist/include/poi \
	  $$( \
	    echo $(BUILD_TYPE) | grep -qi 'debug' && \
	    echo '-g -O0' || \
	    echo '-flto -O3' \
	  ) \
	  -std=c++11 \
	  -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter \
	  -o $@ \
	  $$( (uname -s | grep -qi 'Darwin') || echo '-static' )
