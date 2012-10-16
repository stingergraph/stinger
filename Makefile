INCLUDE=include
CPPFLAGS=-fopenmp

main: main.cpp
	g++ -I$(INCLUDE) $(CPPFLAGS) $^ -o $@
