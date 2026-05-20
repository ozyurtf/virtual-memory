CFLAGS=-g 
CXXFLAGS=-std=c++11
CC=g++-12

mmu: src/mmu.cpp
	$(CC) $(CXXFLAGS) src/mmu.cpp -o mmu

clean:
	rm -f mmu *~

check_version:
	$(CC) --version


