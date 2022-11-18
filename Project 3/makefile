mmpart2:mmpart2.o phyframes.o pagetable.o
	g++ mmpart2.cc pagetable.cc phyframes.cc -o mmpart2

mmpart2.o:mmpart2.cc
	g++ mmpart2.cc -c -Wall -g -o mmpart2.o

phyframes.o:phyframes.cc
	g++ phyframes.cc -c -Wall -g -o phyframes.o

pagetable.o:pagetable.cc
	g++ pagetable.cc -c -Wall -g -o pagetable.o

clean:
	rm *.o mmpart2