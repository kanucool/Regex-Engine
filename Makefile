CXX := g++
CXXFLAGS := -std=c++20 -Wall -Werror

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
TARGET := dfa

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

