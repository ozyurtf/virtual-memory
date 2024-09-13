**Note**: These are the notes that I took to prepare for the midterms of the Operating Systems class at New York University. I used the information from the course slides, Andrew Tanenbaum's Modern Operating Systems book and Claude when I prepare these notes. I tried to create a story and connect all the subtopics with each other as much as possible. And because I tried to write these notes based on my point of view as much as possible, there is no guarantee that every single information in here is correct.

# Memory Management Simulator

In this project, I implemented a memory management simulator that models different page replacement algorithms. The project is designed to simulate how operating systems manage virtual memory and how they handle page faults.

## Features

- Multiple page replacement algorithms are simulated:
  - FIFO (First-In-First-Out)
  - Random
  - Clock
  - ESC (Enhanced Second Chance)
  - Aging
  - Working Set
- Multiple processes with individual page tables are supported.
- Virtual memory management with page tables and frame tables is implemented.
- Different types of memory access operations (read, write) are handled.
- Context switches between processes are simulated.
- Detailed statistics on memory operations are provided.

## Input Files

The simulator requires two input files:
1. A file that describes the processes and their memory access patterns.
2. A file that contains the random numbers that will be used in the Random Page replacement algorithm.

## Implementation Details

- Implemented in C++.
- Various data structures including vectors, deques, and structs to model memory components are used.
- Different paging algorithms as separate structs with a common interface are used.

## Potential Extensions

- Graphical visualization of memory states.
- Support for more page replacement algorithms.
- Parallel simulation of multiple memory management strategies for comparison.

Overall, this project provides a comprehensive simulation of memory management in operating systems. It is useful for educational purposes and understanding how operating systems manage virtual memory.


