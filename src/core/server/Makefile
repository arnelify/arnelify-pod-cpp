# Boot Makefile
# See https://www.gnu.org/software/make/manual/make.html for more about make.

# ENGINE
ENGINE_BUILD = g++
ENGINE_WATCH = clang++
ENGINE_FLAGS = -std=c++2b

# PATH
PATH_BIN = $(CURDIR)/build/index.so
PATH_SRC = $(CURDIR)/src/cpp/ffi.cpp
PATH_TEST_BIN = $(CURDIR)/src/tests/bin/index
PATH_TEST_SRC = $(CURDIR)/src/tests/index.cpp

# INC
INC_CPP = -I $(CURDIR)/src/cpp
INC_INCLUDE = -L /usr/include
INC_JSONCPP = -I /usr/include/jsoncpp/json

INC = ${INC_CPP} ${INC_INCLUDE} ${INC_JSONCPP}

# LINK
LINK_JSONCPP = -ljsoncpp
LINK_ZLIB = -lz
LINK = ${LINK_JSONCPP} ${LINK_ZLIB}

# SCRIPTS
build:
	clear && mkdir -p build && rm -rf build/*
	${ENGINE_BUILD} ${ENGINE_FLAGS} ${INC} ${LINK} -fPIC -shared ${PATH_SRC} -o ${PATH_BIN}

test:
	clear && mkdir -p src/tests/bin && rm -rf src/tests/bin/*
	${ENGINE_WATCH} $(ENGINE_FLAGS) $(PATH_TEST_SRC) ${INC} ${LINK} -o $(PATH_TEST_BIN) && $(PATH_TEST_BIN)

.PHONY: build test