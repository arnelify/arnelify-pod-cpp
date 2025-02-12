# Boot Makefile
# See https://www.gnu.org/software/make/manual/make.html for more about make.

# ENGINE
ENGINE_BUILD = g++
ENGINE_BUILD_FLAGS = -std=c++2b
ENGINE_WATCH = clang++
ENGINE_WATCH_FLAGS = -std=c++2b

# PATH
PATH_WATCH = $(CURDIR)/watch.cpp
PATH_BUILD = $(CURDIR)/server.cpp
PATH_BIN = ../pod/server

# INC
INC_APP = -I $(CURDIR)/app
INC_CORE = -I $(CURDIR)/core
INC_INCLUDE = -L /usr/include
INC_JSONCPP = -I /usr/include/jsoncpp/json
INC = ${INC_APP} ${INC_CORE} ${INC_INCLUDE} ${INC_JSONCPP}

# LINK
LINK_JSONCPP = -ljsoncpp
LINK_ZLIB = -lz
LINK = ${LINK_JSONCPP} ${LINK_ZLIB}

# SCRIPTS
build:
	$(ENGINE_BUILD) $(ENGINE_BUILD_FLAGS) $(PATH_BUILD) ${INC} -o $(PATH_BIN) ${LINK}

watch:
	$(ENGINE_WATCH) $(ENGINE_WATCH_FLAGS) $(PATH_WATCH) ${INC} -o $(PATH_BIN) ${LINK}

.PHONY: build watch