CXX ?= c++
CPPFLAGS += -Iinclude
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -O2
LDFLAGS ?=

ifeq ($(shell uname -s),Darwin)
SDKROOT ?= $(shell test -d /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk && echo /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk)
export SDKROOT
endif

BUILD := build
APP := $(BUILD)/hw1
TESTS := $(BUILD)/tests
SOURCES := $(wildcard src/*.cpp)
HEADERS := $(wildcard include/*.hpp include/*.tpp)
SANITIZERS := -fsanitize=address,undefined -fno-omit-frame-pointer -g
TEST_FLAGS := -std=c++20 -Wall -Wextra -Wpedantic $(SANITIZERS)

.PHONY: all run test sanitize benchmark graph clean help

all: $(APP)

$(BUILD):
	mkdir -p $@

$(APP): $(SOURCES) $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o $@ $(LDFLAGS)

$(TESTS): tests/tests.cpp src/functional_tests.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(TEST_FLAGS) tests/tests.cpp src/functional_tests.cpp -o $@ $(LDFLAGS) $(SANITIZERS)

run: $(APP)
	./$(APP)

test: $(APP)
	./$(APP) --test

benchmark: $(APP)
	./$(APP) --benchmark

sanitize: $(TESTS)
ifeq ($(shell uname -s),Darwin)
	ASAN_OPTIONS=detect_leaks=0 ./$(TESTS)
else
	ASAN_OPTIONS=detect_leaks=1 ./$(TESTS)
endif

graph:
	python3 plot_benchmark.py

clean:
	rm -rf $(BUILD)

help:
	@echo "Targets: all, run, test, sanitize, benchmark, graph, clean"
