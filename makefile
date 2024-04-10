CFLAGS=-g 
CXXFLAGS=-std=c++11
CC=g++-12

mmu: mmu.cpp
	$(CC) $(CXXFLAGS) mmu.cpp -o mmu

clean:
	rm -f mmu *~

check_version:
	$(CC) --version


