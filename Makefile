CXXFLAGS = -std=c++20 -Wall -g
LDLIBS = -lstdc++
.PHONY: all clean
all: parse
clean:
	-rm  parse *.o
parse: parse.o
parse.o: parse.cpp