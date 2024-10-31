# Server Makefile
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
PATH_WATCH_SRC = ./watch.cpp
PATH_BUILD_SRC = ./server.cpp
PATH_BIN = ../pod/server

# INC
INC_APP = -I ./app
INC_CORE = -I ./core
INC_INCLUDE = -L ../include
INC_JSONCPP = -I ../include/jsoncpp/json

INC = ${INC_APP} ${INC_CORE} ${INC_INCLUDE} ${INC_JSONCPP}

# LINK
LINK_JSONCPP = -ljsoncpp
LINK_BOOST = -lboost_system -lboost_thread
LINK_MAGIC = -lmagic
LINK = ${LINK_JSONCPP} ${LINK_BOOST} ${LINK_MAGIC}

# SCRIPTS
build:
	$(CXX) $(CXX_FLAGS) $(PATH_BUILD_SRC) ${INC} -o $(PATH_BIN) ${LINK}

watch:
	$(CXX) $(CXX_FLAGS) $(PATH_WATCH_SRC) ${INC} -o $(PATH_BIN) ${LINK}