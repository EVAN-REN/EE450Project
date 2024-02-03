CXX = g++

CXXFLAGS = -std=c++11

SRCS_DIR = src
SRCS = $(wildcard $(SRCS_DIR)/*.cpp)

OUTPUT_DIR = output

TARGETS = $(SRCS:$(SRCS_DIR)/%.cpp=$(SRCS_DIR)/%)

all: $(TARGETS)

$(SRCS_DIR)/%: $(SRCS_DIR)/%.cpp
	$(CXX) $< -o $@

clean:
	rm -f $(TARGETS)

.PHONY : all clean