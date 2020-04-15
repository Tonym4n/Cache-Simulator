cache-sim:	cache-sim.o
		g++ -g -Wall -std=c++17 cache-sim.o -o cache-sim

cache-sim.o:	cache-sim.cpp
		g++ -g -Wall -std=c++17 -c cache-sim.cpp

clean:		
		rm -f *~ *.o *.gch cache-sim
