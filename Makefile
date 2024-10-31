# Boot Makefile
# See https://www.gnu.org/software/make/manual/make.html for more about make.
#
# For VSCode or Visual Studio Code.
# Add the following to your IncludePath settings:
#
# C/C++ IntelliSense Plugin > Settings > Clang_format_fallback Style > Google
#
#	"includePath": [
#		"${workspaceFolder}/src/app",
#		"${workspaceFolder}/src/core",
#		"${workspaceFolder}/include/jsoncpp/json",
#		"${workspaceFolder}/include"
#	],
# "cppStandard": "c++23",
# "cStandard": "c23",

# CXX
CXX = g++
CXX_FLAGS = -std=c++23

# PATH
PATH_SRC = ./src/core/boot/index.cpp
PATH_BIN = ./src/core/boot/bin/index

# INC
INC_APP = -I ./app
INC_CORE = -I ./src/core
INC_INCLUDE = -L ./include
INC_JSONCPP = -I ./include/jsoncpp/json

INC = ${INC_APP} ${INC_CORE} ${INC_INCLUDE} ${INC_JSONCPP}

# LINK
LINK_JSONCPP = -ljsoncpp
LINK = ${LINK_JSONCPP}

BEFORE = clear

# SCRIPTS
setup:
	${BEFORE} && $(CXX) $(CXX_FLAGS) $(PATH_SRC) ${INC} -o $(PATH_BIN) ${LINK} && $(PATH_BIN) setup

build:
	${BEFORE} && $(CXX) $(CXX_FLAGS) $(PATH_SRC) ${INC} -o $(PATH_BIN) ${LINK} && $(PATH_BIN) build

watch:
	${BEFORE} && $(CXX) $(CXX_FLAGS) $(PATH_SRC) ${INC} -o $(PATH_BIN) ${LINK} && $(PATH_BIN) watch

migrate:
	${BEFORE} && $(CXX) $(CXX_FLAGS) $(PATH_SRC) ${INC} -o $(PATH_BIN) ${LINK} && $(PATH_BIN) migrate

seed:
	${BEFORE} && $(CXX) $(CXX_FLAGS) $(PATH_SRC) ${INC} -o $(PATH_BIN) ${LINK} && $(PATH_BIN) seed