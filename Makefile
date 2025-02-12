# Boot Makefile
# See https://www.gnu.org/software/make/manual/make.html for more about make.

# ENGINE
ENGINE = clang++
ENGINE_FLAGS = -std=c++2b

# PATH
PATH_SRC = $(CURDIR)/src/core/boot/index.cpp
PATH_BIN = $(CURDIR)/src/core/boot/bin/index

# INC
INC_CORE = -I $(CURDIR)/src/core
INC_INCLUDE = -L /usr/include
INC_JSONCPP = -I /usr/include/jsoncpp/json
INC = ${INC_CORE} ${INC_INCLUDE} ${INC_JSONCPP}

# LINK
LINK_JSONCPP = -ljsoncpp
LINK = ${LINK_JSONCPP}

# SCRIPTS
setup:
	clear && mkdir -p src/core/boot/bin
	clear && $(ENGINE) $(ENGINE_FLAGS) $(PATH_SRC) ${INC} -o $(PATH_BIN) ${LINK} && $(PATH_BIN) setup

build:
	clear && mkdir -p src/core/boot/bin
	clear && $(ENGINE) $(ENGINE_FLAGS) $(PATH_SRC) ${INC} -o $(PATH_BIN) ${LINK} && $(PATH_BIN) build

watch:
	clear && mkdir -p src/core/boot/bin
	clear && $(ENGINE) $(ENGINE_FLAGS) $(PATH_SRC) ${INC} -o $(PATH_BIN) ${LINK} && $(PATH_BIN) watch

migrate:
	clear && mkdir -p src/core/boot/bin
	clear && $(ENGINE) $(ENGINE_FLAGS) $(PATH_SRC) ${INC} -o $(PATH_BIN) ${LINK} && $(PATH_BIN) migrate

seed:
	clear && mkdir -p src/core/boot/bin
	clear && $(ENGINE) $(ENGINE_FLAGS) $(PATH_SRC) ${INC} -o $(PATH_BIN) ${LINK} && $(PATH_BIN) seed

.PHONY: setup build watch migrate seed