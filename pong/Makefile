########################
# External dependencies.
########################

LIBEVENT = lib/libevent-2.0.6-rc/build

############
# Main show.
############

CXX := g++
CXXFLAGS := -Wall -pedantic -I$(LIBEVENT)/include
LDFLAGS := -levent -L$(LIBEVENT)/lib
BUILD := build/release

DEBUG ?= 1
ifeq ($(DEBUG), 1)
	CXXFLAGS += -DDEBUG -g -Wextra
	BUILD := build/debug
else
	CXXFLAGS += -O3
endif

all: main

.PHONY: all clean

clean:
	rm -f *.o
	rm -rf build
	mkdir -p build/release
	mkdir -p build/debug

###############
# Real rules. #
###############

main: src/main.cpp $(BUILD)/pong.o $(BUILD)/util.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $(BUILD)/$@

$(BUILD)/%.o: src/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@
