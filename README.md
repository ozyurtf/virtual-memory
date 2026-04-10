# Virtual Memory Simulator

In this project, I implemented a virtual memory simulator that models different page replacement algorithms. The goal behind the project was simulating how operating systems manage virtual memory and how they handle page faults. Almost everything (including the entire C++ implementation) was written by me 100% manually. Only the simulation of how my implementation works was created with Claude Code with my guidance. 

## Demo 

[![Demo](/figures/virtual-memory.png)](https://www.youtube.com/watch?v=NNNUpGNWaA8)

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

