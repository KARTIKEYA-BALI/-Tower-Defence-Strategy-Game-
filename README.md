# Siege and Supply
 
A tower defence strategy game powered by search, routing and compression engines.
 
Team **Bit Beaters** | Project ID DSCPP-III-2026-T016 | B.Tech DSCPP-III
 
The project is led by Kartikeya Bali.
 
---
 
## Overview
 
Siege and Supply is a 2D tower defence game written in C and C++. The player places towers on a grid to stop waves of enemies reaching the exit, and supply drones fly ammunition out to towers that have run dry. Every tower placed changes the shape of the map, so the enemies have to work out a new way through. The grid, the event queue, the pathfinder, the shop search index and the save compressor are all written by hand, with no game engine, database or networking.
 
---
 
## Motivation
 
In coursework we meet queues in one assignment, trees in another and shortest paths in a third, each sitting alone in its own small program. A game forces them into the same process at the same time, with the state of the world changing under them while they run.
 
---
 
## Architecture
 
The project has three layers. Commands move down and results come back up.
 
```
+-------------------------------------------------------------+
|  Presentation layer  (C++ / SDL2)                            |
|  window, renderer, grid, shop panel, ammo bars, input        |
+-------------------------------------------------------------+
                |  input events                ^  render data
                v                              |
+-------------------------------------------------------------+
|  Entity layer  (C++)                                         |
|  Tower, Enemy, Projectile, SupplyDrone, managers, game loop  |
+-------------------------------------------------------------+
                |                              ^
                |   engine_core.h  extern "C"  |
                v                              |
+-------------------------------------------------------------+
|  Engine layer  (C)                                           |
|  grid + event queue | route optimiser | trie index | huffman |
+-------------------------------------------------------------+
                |                              ^
                v                              |
        level and wave files            compressed save file
```
 
The C layer owns the data structures and algorithms. The C++ layer owns the objects the player can see and the rules that govern them. The SDL2 layer owns pixels and input, and holds no game state.
 
---
 
## The C and C++ boundary
 
C++ mangles function names during compilation so that overloading can work, and C does not. The two sides therefore meet in one header marked `extern "C"`, which tells the C++ compiler to leave those names alone.
 
That header declares everything C++ is allowed to ask of the engine and passes only plain C types, so no class ever crosses the line and there is one file to check when something fails to link.
 
---
 
## Core components
 
### 1. Route optimiser: graph, Dijkstra and a min-heap
 
Each walkable cell is a vertex and each legal move to a neighbour is an edge, weighted by what that move costs the mover. Dijkstra's algorithm keeps a tentative distance for every vertex, repeatedly settles the nearest unsettled one and relaxes its outgoing edges. Its distances are final once settled as long as no weight is negative, and all our costs are positive. Choosing the nearest vertex is the step that decides the speed: a linear scan makes the search O(V²), while a binary min-heap makes each extraction O(log V) and brings the whole thing to O((V + E) log V).
 
Enemies ask for a route when they spawn, and again whenever a tower changes the walkability of a cell. Before a placement is accepted the engine checks that a route from the spawn to the exit still exists, so the player cannot seal the map completely. Drones call the same function with a different cost table, one that raises the price of cells under fire and lets a longer quiet route come out cheaper than a short dangerous one.
 
### 2. Tower shop search: a trie with a ranking heap
 
A trie stores names by their characters. Each edge is a character, and a path from the root spells out a prefix. Walking to the node for what the player has typed costs one step per character, and the number of towers in the shop does not change that cost. Every name the player could be reaching for sits in the subtree below that node, so a depth-first walk collects the candidates and a heap keyed on a ranking score returns the strongest few.
 
Tower names are inserted into the trie once at load time from the definition file. The search box queries the engine on every keystroke and gets back tower ids, which the C++ layer turns into icons, costs and stats. Picking a suggestion arms the placement cursor for the next click.
 
### 3. Compression engine: Huffman coding for saves
 
A save holds the grid, every tower and its upgrade state, the enemies still alive, the wave number, the player's resources and the ammunition levels. The same bytes repeat constantly in it, because most of a grid is empty ground and most towers are the same few types. Huffman coding counts how often each byte appears, puts one leaf per byte into a min-heap keyed by frequency, then joins the two smallest under a new parent until a single tree is left. Reading down that tree gives every byte a code, short for common bytes and long for rare ones. Because each byte sits at a leaf, no code is a prefix of another, so the decoder can read a stream of bits with no separators in it and still know where each symbol ends.
 
The save manager flattens the live objects into a buffer and hands it to the engine, which writes the frequency table followed by the packed bits. The loader rebuilds an identical tree from that table, so the tree itself never has to be stored. A round-trip test saves a game, loads it, saves it again and compares the two buffers byte for byte.
 
The min-heap used here is the same one written for the route optimiser.
 
### 4. Object-oriented design in C++
 
Everything the player can see on the field derives from one small base class holding a grid position and a health value, with two operations every subclass supplies: advance yourself by one time step, and draw yourself.
 
Inheritance lets Tower, Enemy, Projectile and SupplyDrone share position and lifetime handling instead of repeating it four times. Polymorphism lets the game loop keep one container of base class pointers and advance all of them through the same call, with no switch on a type tag, so a new enemy type does not mean editing the loop. Encapsulation keeps health and ammunition private, so a tower cannot end up with negative ammunition. Composition builds the larger objects from smaller ones: a tower has an ammunition store and a targeting module, and a drone has the route the engine handed back. Two managers wrap the trie and the compressor so the rest of the C++ code never calls the engine directly.
 
### 5. SDL2 for display and input
 
SDL2 is a C library giving a program a window, a 2D renderer, an event queue for mouse and keyboard, and timing, behind one interface that works on Windows, Linux and macOS. We use a small part of it: a window and renderer at start-up, then each frame we drain the event queue, draw sprites as textures and fill rectangles for the ammunition bars and grid overlay.
 
A frame runs in one direction. SDL2 hands over a click or a keypress, which is converted to a grid cell or a command and passed to the entity layer. That layer decides what the command means and calls the engine for anything needing an algorithm. The engine returns a plain result, an array of cells or ids, and the entities update themselves from it before the renderer draws the frame. The presentation layer never touches game state, which keeps the logic testable without opening a window.
 
---
 
## Building
 
A C11 and C++17 toolchain (GCC or Clang), the SDL2 development libraries and Git. The C and C++ sources are compiled separately and linked in one step. Build instructions go here once the first Makefile is committed.
 
---
 
## Roadmap
 
The work is ordered so that every stage ends with a version of the game that runs, even when it is far from finished.
 
- [ ] Core in C. Grid representation and event queue.
- [ ] Route optimiser. Graph, min-heap and Dijkstra, checked against fixed test maps.
- [ ] Playable base game. Towers, enemies, waves, firing and placement.
- [ ] Tower shop search. Trie index and ranked suggestions in the shop panel.
- [ ] Supply drones. Ammunition and reload events, drones routed around danger.
- [ ] Save and load. Huffman compression with the round-trip check.
- [ ] Polish and testing. Edge cases, bug fixes, timing measurements.
---
 
## Testing
 
Each engine module is tested on its own before it is wired into the game. The heap is checked against a sorted array. Dijkstra is checked on small maps with hand-worked answers, including one map with no route at all. The trie is checked on prefixes matching none, one and many names, and the compressor by round trip. The assembled game is then played through a full level to confirm the parts still behave together.
 
---
 
## Scope
 
Single player, fixed grid, desktop with a keyboard and mouse. Level data, enemy waves and tower statistics are read from local files and saves are written to local disk, so nothing needs an internet connection. There is no multiplayer, no server and no external live data.
 