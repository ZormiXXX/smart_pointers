CXX ?= c++
CPPFLAGS += -Iinclude
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -O2
LDFLAGS ?=

# Command Line Tools on this Mac ship a malformed 27.0 SDK; prefer the working 26.5 SDK.
ifeq ($(shell uname -s),Darwin)
SDKROOT ?= $(shell if [ -d /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk ]; then echo /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk; fi)
ifneq ($(strip $(SDKROOT)),)
export SDKROOT
endif
endif

BUILD_DIR := build
PROGRAM := $(BUILD_DIR)/hw1
TEST_PROGRAM := $(BUILD_DIR)/tests
SANITIZER_FLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer -g

.PHONY: all run test benchmark sanitize graph sources-txt clean help

all: $(PROGRAM)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

HEADERS := $(wildcard include/*.hpp) $(wildcard include/*.tpp)
APP_SOURCES := src/main.cpp src/ui.cpp src/benchmark.cpp src/functional_tests.cpp
TEST_SOURCES := tests/tests.cpp src/functional_tests.cpp

$(PROGRAM): $(APP_SOURCES) $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(APP_SOURCES) -o $@ $(LDFLAGS)

$(TEST_PROGRAM): $(TEST_SOURCES) $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) -std=c++20 -Wall -Wextra -Wpedantic $(SANITIZER_FLAGS) $(TEST_SOURCES) -o $@ $(LDFLAGS) $(SANITIZER_FLAGS)

run: $(PROGRAM)
	./$(PROGRAM)

test: $(PROGRAM)
	./$(PROGRAM) --test

benchmark: $(PROGRAM)
	./$(PROGRAM) --benchmark

sanitize: $(TEST_PROGRAM)
ifeq ($(shell uname -s),Darwin)
	ASAN_OPTIONS=detect_leaks=0 ./$(TEST_PROGRAM)
else
	ASAN_OPTIONS=detect_leaks=1 ./$(TEST_PROGRAM)
endif

graph:
	python3 plot_benchmark.py

sources-txt:
	python3 export_sources.py

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Targets: all, run, test, benchmark, sanitize, graph, clean"
