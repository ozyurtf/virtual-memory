**Note**: In this project, I implemented a virtual memory simulator that models different page replacement algorithms. The goal behind the project was simulating how operating systems manage virtual memory and how they handle page faults. Almost everything (including the entire C++ implementation) was written by me 100% manually. Only the simulation of how my implementation works was created with Claude Code with my guidance. 

## Demo

<div style="position:relative;padding-bottom:56.25%;height:0;overflow:hidden;max-width:100%">
<iframe style="position:absolute;top:0;left:0;width:100%;height:100%;border:0"
  src="https://www.youtube.com/embed/NNNUpGNWaA8?si=O2IM57heqJ_NAgiF"
  title="YouTube video player"
  allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share"
  referrerpolicy="strict-origin-when-cross-origin"
  allowfullscreen></iframe>
</div>

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
