# Siege and Supply

A tower defence strategy game powered by search, routing and compression engines.

Team **Bit Beaters** | Project ID `DSCPP-III-2026-T016` | B.Tech DSCPP-III

Siege and Supply is a 2D tower defence game written in C and C++. The player places towers on a grid to stop waves of enemies from reaching the exit, while supply drones fly ammunition out to towers that have run dry. Everything underneath the game is written by hand: the grid, the event queue, the pathfinder, the shop search index and the save file compressor. We are not using a game engine, a database or any networking.

The reason for building it this way is simple. In coursework we meet queues in one assignment, trees in another and shortest paths in a third, and each one lives alone in its own small program. A game forces them into the same process at the same time, with the state of the world changing under them while they run, which is where the trade-offs between them actually show up.

## Team

| Role | Name | Roll number |
|------|------|-------------|
| Team Lead | Kartikeya Bali | 2510370312 |
| Member | Parthvi Rawat | 2510012270 |
| Member | Isha Bhandari | 2510380218 |
| Member | Sumedha Thakur | 2510370247 |

Kartikeya Bali leads the team and coordinates the work across the three layers described below.

## What the game does

The map is a fixed grid. Enemies spawn at one edge and try to walk to the exit. The player spends resources to place towers, and every tower that goes down changes the shape of the map, so the enemies have to work out a new way through. Towers fire and consume ammunition. When a tower runs low, a supply drone is dispatched to it, and the drone has to plan its own route while staying away from squares under fire.

The drone and the enemy both ask the same pathfinder for a route, and both get an answer shaped by their own movement rules. That shared call is the part of the design we expect to spend the most time on.

## Architecture

The project has three layers. Data moves down as commands and comes back up as results.

```
+-------------------------------------------------------------+
|  Presentation layer  (C++ / SDL2)                            |
|  window, renderer, grid drawing, shop panel, ammo bars       |
|  mouse and keyboard input                                    |
+-------------------------------------------------------------+
                |  input events                ^  render data
                v                              |
+-------------------------------------------------------------+
|  Entity layer  (C++)                                         |
|  Tower, Enemy, Projectile, SupplyDrone                       |
|  ShopSearch, SaveManager, game loop                          |
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

The C layer owns the data structures and the algorithms. The C++ layer owns the objects the player can see and the rules that govern them. The SDL2 layer owns pixels and input, and it holds no game state at all. If the presentation layer were deleted, the game would still be able to run a full wave with its state printed to a terminal.

### The C and C++ boundary

C++ mangles function names during compilation so that overloading can work. C does not. Linking the two together therefore needs a declaration that tells the C++ compiler to leave the names alone, and that is what `extern "C"` does.

We keep the entire boundary in one header, `engine_core.h`, so there is exactly one file to look at when something fails to link.

```c
/* engine_core.h */
#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { int x, y; } CellPos;

int  ec_grid_init(int width, int height);
int  ec_route_find(CellPos from, CellPos to, int mover_kind,
                   CellPos *out_path, int max_len);
int  ec_shop_suggest(const char *prefix, int *out_ids, int max_results);
long ec_save_compress(const unsigned char *state, long len,
                      const char *out_path);

#ifdef __cplusplus
}
#endif
#endif
```

Every C++ file includes that header and nothing else from the engine. The C sources never include a C++ header, and they never see a class.

## Core components

### 1. Route optimiser: graph, Dijkstra and a min-heap

Each walkable cell on the grid becomes a vertex. Each legal move from a cell to a neighbour becomes an edge, and the weight of that edge is what the move costs the mover. For a plain enemy walking over open ground the cost is uniform. For a supply drone the cost of a cell rises when it sits inside a tower's firing arc or an enemy's attack range, so a longer quiet route can end up cheaper than a short dangerous one.

Dijkstra's algorithm settles vertices in order of their distance from the source. It keeps a tentative distance for every vertex, repeatedly takes the unsettled vertex with the smallest tentative distance, and relaxes its outgoing edges. Once a vertex is taken off the frontier its distance is final, which holds as long as no edge weight is negative. Our costs are all positive, so the condition is satisfied.

Picking "the unsettled vertex with the smallest distance" is the operation that decides how fast the whole thing runs. A linear scan over every vertex makes it O(V²). A binary min-heap makes each extraction O(log V) and brings the algorithm down to O((V + E) log V). We write the heap ourselves in C, as an array with the usual sift-up and sift-down pair, and expose `push`, `pop_min` and `decrease_key`.

How it plugs into the game:

- Enemies request a route from their current cell to the exit when they spawn.
- Placing or selling a tower changes the walkability of a cell, which changes the graph. The engine raises a `MAP_CHANGED` event, and every enemy currently on the field asks for a fresh route.
- Before a placement is accepted, the engine runs a route query from the spawn to the exit. If no route comes back, the placement is rejected and the player keeps their resources, because a fully sealed map would leave the enemies with nowhere to go.
- Supply drones call the same function with a different mover kind, which switches the cost table, so both movers share one implementation.

The min-heap written for this component is reused by the Huffman encoder described below, which is a good demonstration of the same structure doing unrelated work.

### 2. Tower shop search: a trie with a ranking heap

The shop has a search box. As the player types, a short list of matching towers appears under the cursor, and the list narrows with every character.

A trie stores strings by their characters. The root is empty, each edge is a character, and a path from the root spells out a prefix. A node carries a flag saying whether a complete name ends there, plus the tower id if one does. Looking up a prefix costs one step per character typed. The number of towers stored in the shop does not change that cost, which is the property that makes the structure worth using here.

Once we have walked to the node for the typed prefix, every name the player could be reaching for sits in the subtree below it. A depth-first walk of that subtree collects the candidates. We then push them into a max-heap keyed by a ranking score stored with each tower, and pop the top few. The player sees the strongest matches first instead of the alphabetically earliest ones.

How it plugs into the game:

- Every tower name is inserted into the trie once, at load time, from the tower definition file.
- The C++ `ShopSearch` class holds the search box state and calls `ec_shop_suggest()` on each keystroke.
- The engine returns tower ids, and the C++ layer looks up the icon, cost and stats for display. The C layer never knows what a tower looks like.
- Selecting a suggestion arms the placement cursor, and the next click on a valid cell builds the tower.

### 3. Compression engine: Huffman coding for saves

A saved game is a byte stream describing the grid, every tower and its upgrade state, the enemies still alive, the wave number, the player's resources and the ammunition levels. Written out plainly, the same bytes repeat constantly, because most of a grid is empty ground and most towers are the same few types.

Huffman coding takes advantage of that. It counts how often each symbol appears, then gives short bit codes to common symbols and long ones to rare symbols. The tree is built from the bottom up:

1. Count the frequency of every byte in the state buffer.
2. Put one leaf node per distinct byte into a min-heap keyed by frequency.
3. Pop the two smallest nodes, join them under a new parent whose frequency is their sum, and push the parent back.
4. Repeat until one node is left. That node is the root.
5. Walk the tree, adding `0` for a left branch and `1` for a right branch. The bits collected on the way to a leaf are that byte's code.

Because every real byte sits at a leaf, no code is ever a prefix of another code. That is what lets the decoder read a stream of bits with no separators in it and still know where each symbol ends. Decoding walks the tree one bit at a time, emits a byte when it lands on a leaf, and jumps back to the root.

A hash table maps each byte to its finished code during encoding, so writing out a symbol is a constant-time lookup.

How it plugs into the game:

- The C++ `SaveManager` serialises the live game objects into a flat buffer and hands it to the engine.
- The engine writes a header containing the frequency table, followed by the packed bit stream. The loader rebuilds an identical tree from that frequency table, so the tree itself never has to be stored.
- Loading decodes the stream and hands the buffer back to `SaveManager`, which reconstructs the objects.
- Our round-trip test saves a game, loads it, serialises it again and compares the two buffers byte for byte. Anything other than an exact match is a bug.

We will report the raw and compressed sizes of the sample saves so the effect of the coding is visible in numbers.

### 4. Object-oriented design in C++

The entity layer is where the OOP work happens. Everything the player can see on the field derives from one small base class.

```cpp
class Entity {
public:
    virtual ~Entity() = default;
    virtual void update(float dt) = 0;
    virtual void render(SDL_Renderer* r) const = 0;
    bool alive() const { return hp_ > 0; }
protected:
    CellPos pos_;
    int hp_ = 1;
};

class Tower : public Entity {
public:
    void update(float dt) override;      // acquire target, fire, spend ammo
    void render(SDL_Renderer* r) const override;
    bool needsResupply() const { return ammo_ < reorderLevel_; }
private:
    AmmoStore  ammo_;                    // composition
    Targeting  targeting_;               // composition
    int        reorderLevel_;
};
```

Inheritance lets `Tower`, `Enemy`, `Projectile` and `SupplyDrone` share position handling, health and lifetime management without copying that code four times.

Polymorphism lets the game loop hold one container of `Entity*` and call `update()` on all of them. There is no switch on a type tag anywhere in the loop, and adding a new enemy type does not mean editing the loop.

```cpp
for (auto& e : entities_) e->update(dt);
for (auto& e : entities_) e->render(renderer_);
entities_.erase(std::remove_if(entities_.begin(), entities_.end(),
                               [](const auto& e){ return !e->alive(); }),
                entities_.end());
```

Encapsulation keeps health and ammunition private. A tower cannot end up with negative ammunition because nothing outside the class can write to the counter, and the only way to spend a round is through a method that checks first.

Composition builds the larger objects out of smaller ones. A tower has an ammunition store and a targeting module. A drone has a route, which is the array of cells the C engine handed back. Two managers, `ShopSearch` and `SaveManager`, wrap the trie and the compressor so the rest of the C++ code never calls the engine directly.

### 5. SDL2 for display and input

SDL2 is a C library that gives a program a window, a hardware-accelerated 2D renderer, an event queue for the mouse and keyboard, and timing functions, all behind one interface that works on Windows, Linux and macOS. It draws what we tell it to draw. It knows nothing about towers.

We use a small part of it: `SDL_CreateWindow` and `SDL_CreateRenderer` at start-up, `SDL_PollEvent` to drain input each frame, textures for the sprites and `SDL_RenderCopy` to place them, plus `SDL_RenderFillRect` for the ammunition bars and the grid overlay.

One frame of the game runs like this:

1. `SDL_PollEvent` drains the queue. A click is converted from pixel coordinates to a grid cell, and a keypress is converted to a command. Nothing else happens here.
2. The command goes to the entity layer, which decides what it means. A click with the placement cursor armed becomes a build request. A keystroke in the shop box becomes a search.
3. The entity layer calls the C engine through `engine_core.h` for the parts that need an algorithm: a route, a list of suggestions, a save.
4. The engine returns a plain result. An array of cells, an array of ids, a byte count.
5. Entities update their own state from that result, and the renderer draws the new frame.

Input travels in one direction and render data travels back in the other. The presentation layer never modifies the game state itself, which keeps the interesting logic testable without opening a window at all.

## Repository layout

This is the layout we are building towards. Directories appear as the matching milestone lands.

```
siege-and-supply/
├── src/
│   ├── engine/            # C: grid, event queue, heap, dijkstra, trie, huffman
│   │   ├── engine_core.h  # the single extern "C" boundary
│   │   ├── grid.c
│   │   ├── events.c
│   │   ├── minheap.c
│   │   ├── route.c
│   │   ├── trie.c
│   │   └── huffman.c
│   ├── game/              # C++: entities, managers, game loop
│   │   ├── Entity.hpp
│   │   ├── Tower.cpp
│   │   ├── Enemy.cpp
│   │   ├── SupplyDrone.cpp
│   │   ├── ShopSearch.cpp
│   │   └── SaveManager.cpp
│   └── ui/                # C++/SDL2: window, renderer, input, panels
├── assets/                # sprites and fonts
├── levels/                # map layouts and wave definitions (plain text)
├── saves/                 # compressed save files
├── tests/                 # per-module tests and the round-trip check
└── docs/                  # proposal, report, diagrams
```

## Building

Requirements:

| Tool | Version |
|------|---------|
| C compiler | C11, GCC or Clang |
| C++ compiler | C++17, GCC or Clang |
| SDL2 | development headers and libraries |
| Git | any recent version |

The C sources are compiled with the C compiler, the C++ sources with the C++ compiler, and the two sets of objects are linked together in one step. Build instructions go here once the first Makefile is committed.

## Roadmap

Each phase ends with a version of the game that runs, even when it is far from finished.

- [ ] Phase 1, core in C. Grid representation and event queue, with tests for both.
- [ ] Phase 2, route optimiser. Graph over the grid, min-heap and Dijkstra, checked against fixed test maps with known answers.
- [ ] Phase 3, playable base game. Towers, enemies, waves, firing and placement through the C++ entity layer. First version playable from start to finish.
- [ ] Phase 4, tower shop search. Trie index and ranked suggestions in the shop panel.
- [ ] Phase 5, supply drones. Ammunition and reload events, drones routed around dangerous cells.
- [ ] Phase 6, save and load. Huffman compression of the full game state, with the round-trip check.
- [ ] Phase 7, polish and testing. Edge cases, bug fixes, timing measurements, final build.

## Testing

Every engine module is tested on its own before it is wired into the game. The heap is checked against a sorted array. Dijkstra is checked on small maps where the shortest path can be worked out by hand, including a map with no valid route. The trie is checked for prefixes that match nothing, match one name and match many. The compressor is checked by round trip, including on an empty buffer and on a buffer of one repeated byte. After that, the assembled game is played through a full level to confirm the parts still behave together.

## Scope and assumptions

Siege and Supply is single player, on a grid of fixed size, on a desktop computer with a keyboard and a mouse. Level data, enemy waves and tower statistics are read from local files. Nothing in the game needs an internet connection, and changing battlefield conditions are simulated inside the program. The shop holds few enough towers to keep the whole list in memory while the game runs, and saves are written to local disk. We assume a working C and C++ toolchain with SDL2 available on the machines we develop and demonstrate on. There is no multiplayer, no server and no external live data.

## References

1. Cormen, T. H., Leiserson, C. E., Rivest, R. L. and Stein, C., *Introduction to Algorithms*.
2. Dijkstra, E. W. (1959). A note on two problems in connexion with graphs.
3. Huffman, D. A., "A Method for the Construction of Minimum-Redundancy Codes", *Proceedings of the IRE*, Volume 40, Number 9, 1952, pages 1098-1101.
4. Sedgewick, R. and Wayne, K., *Algorithms*, 4th edition, Addison-Wesley, 2011.
5. Stroustrup, B., *The C++ Programming Language*, 4th edition, Addison-Wesley, 2013.
6. Kernighan, B. W. and Ritchie, D. M., *The C Programming Language*, 2nd edition, Prentice Hall, 1988.
7. SDL2 Documentation Wiki, Simple DirectMedia Layer. https://wiki.libsdl.org
8. C++ Reference. https://en.cppreference.com
