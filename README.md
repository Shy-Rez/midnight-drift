# Midnight Drift 
**A high-speed, procedurally generated retro arcade raceing game built entirely in C++ and SFML.**

This game focuses on performance, custom physics, math, and infinite replayability. It features time bending mechanics, drift handling, and a clone system that lets you race your own past personal record for a track.

## Technical highlights

The core systems ere built from the ground up to showcase algorithm design and optimization:

* **Procedural Matrix Generation:** The tracks aren't hand drawn. An algorithm translates 10 digit seeds into a playable 2D matrix array, ensuring an infinite number of unique layouts. The same seed always produces the same track on any machine.
* **Surface-Based Physics:** The engine handles different terrain types dynamically. It calculates grip coefficients based on the surface under the car, requiring the player to adjust their driving style for different surfaces. Asphalt allows long controlled drifts, ice offers near zero lateral correction, sticky surfaces snap the car back to its line, and gravel creates loose unpredictable slides.
* **Custom Drift Physics:** No pre built physics engines. The forward velocity, drifting mechanic, and frame independent momentum as built entirely using C++ standard math.
* **Ghost Replay System:** The system captures your coordinates and rotation data every single frame. When a personal best is set, that buffer is promoted and used to drive a translucent ghost car on subsequent laps, a perfect replay of your fastest run with no physics overhead.
* **Time Dilation (Slow-Mo):** A custom delta time matrix allows the player to hold Shift and instantly drop the game simulation speed to 35%, scaling the camera zoom, physics, and particle effects.
* **Optimized Rendering:** The tile grid is never iterated in full. Each frame visits only the tiles within a viewport-sized window. An underglow fades quadratically from the car position, calculating per-tile color boosts only within a fixed radius.

## Tech stack
* **Language:** C++
* **Graphics API:** SFML (Simple and Fast Multimedia Library)
* **Build System:** CMake 

## How to play
| Action | Keybinding |
| :--- | :--- |
| **Accelerate / Brake** | `W` / `S` |
| **Steer** | `A` / `D` |
| **Engage Slow-Mo Matrix**| Hold `Left Shift` |
| **Save Track Seed** | Press `M` (While racing) |
| **Quick Restart** | Press `R` (While racing) |

### Tips:

* Brake before the apex, not during. The drift model rewards early corner entry.
* Ice sections have almost no lateral recovery steer into the slide before it starts.
* Sticky patches can be used deliberately to cut tight corners without spinning out.
* The ghost appears after your first completed lap. Treat it as a braking and entry reference.
* Slow-mo scales the lap timer alongside the physics, it is a skill tool gto navigate difficult portions.

## Project structure

```text
midnight-drift/
├── src/                  # Core source code and headers
│   ├── main.cpp          # Main game loop, state machine, and UI
│   ├── car.cpp / .h      # Vehicle physics and ghost car memory
│   ├── track.cpp / .h    # Procedural map generation algorithms
│   ├── physics.h         # Advanced math, lerping, and vector logic
│   ├── hud.cpp / .h      # Minimap, timers, and stylized text rendering
│   ├── fx.h              # Particle systems (sparks, flames, tire marks)
│   └── constants.h       # Global configurations
├── CMakeLists.txt        # Build instructions
├── README.md
└── .gitignore           
```

## How to run

Make sure CMake and SFML are installed in your system to build the game.
Donload the latest versions from here.
**CMake (Build Tool):** https://cmake.org/download/
**SFML (Graphics Library):** https://www.sfml-dev.org/download/

Once the tools above are installed, open your terminal in the midnight-drift directory.

**Using the Terminal:**
Open your terminal in the root `midnight-drift` directory and run:

```bash
#Generate the build environment
mkdir build && cd build
cmake ..

#Compile the source code
cmake --build .

#Launch the game
.\midnight-drift.exe       
```

Note: If you get a 'missing .dll' error, copy the files from your SFML bin/ folder and paste them into your build/ folder where the .exe was created.



## Future Goals

* **Multiplayer Mode:** Implementing UDP socket communication through SFML's networking module for real time LAN racing. Both machines generate the same track from a shared seed. Only small state packets like position, angle, lap progress, etc. are sent each frame.

* **Track Editor:** A drag and drop in game tool to place and adjust waypoints manually. Edited layouts feed back into the same pipeline as procedural tracks, producing a sharable seed rather than a custom file format.

* AI Opponents: Waypoint following agents with lookahead steering and configurable aggression. Speed scales to the player's current lap time to maintain competitive pressure without feeling unfair.
  
* Web Integration: Emscripten compilation through CMake so the game runs in a browser without installation. The deterministic seed system means a seed entered on desktop produces an identical circuit on web.

* Career Mode: A structured campaign with a fixed sequence of seeded tracks, time gate unlocks, and persistent lap records. Target times gate access to harder circuit categories with tighter choke points and narrow tracks.
