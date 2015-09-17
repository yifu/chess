
# n.o is made automatically from n.cc, n.cpp, or n.C with a recipe of
# the form ‘$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c’
CXXFLAGS=-Werror -Wall -Wextra -Wpedantic -ggdb -std=c++11 $(shell sdl2-config --cflags)



# n is made automatically from n.o by running the linker (usually
# called ld) via the C compiler. The precise recipe used is ‘$(CC)
# $(LDFLAGS) n.o $(LOADLIBES) $(LDLIBS)’. [...]
CC=g++
LDFLAGS=
LOADLIBES=
# From stackoverflow
# [http://stackoverflow.com/questions/17052006/make-ldlibs-deprecated]:
# "Non-library linker flags, such as -L, should go in the LDFLAGS
# variable."


# [...]This rule does the right thing for a simple program with only
#one source file. It will also do the right thing if there are
#multiple object files (presumably coming from various other source
#files), one of which has a name matching that of the executable
#file. Thus, x: y.o z.o when x.c, y.c and z.c all exist[...]

.PHONY: debug release clean

debug: CXXFLAGS += -DDEBUG
debug: chessboard server

release: CXXFLAGS += -DNDEBUG -O3
release: chessboard server

clean:
	rm server chessboard *.o

utils.o: utils.hpp utils.cpp
game.o: game.hpp game.cpp net_protocol.hpp
net_protocol.o: net_protocol.hpp

chessboard: LDLIBS=$(shell sdl2-config --libs) -lSDL2_image -lSDL2_ttf
chessboard: utils.o game.o net_protocol.o

server: LDLIBS=
server: game.o net_protocol.o
