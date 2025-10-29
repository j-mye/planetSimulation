# Planetary Simulation

***An interactive N-body planetary simulation and renderer built with C++, OpenGL (GLFW + GLAD), GLM, and ImGui.***

This repository contains a simulation of gravitational bodies with an OpenGL renderer that supports trails, glow, and a background starfield. The GUI provides controls for simulation parameters and camera interaction.

## Highlights / Key Features

- Real-time N-body simulation with configurable time scaling and gravity parameters.
- Smooth camera that follows system center-of-mass (COM) or a selected planet.
- Persistent manual zoom offset: manual zoom (buttons) now persists relative to the auto-fit zoom.
- Double-click a planet to follow it, double-click again to return to COM follow.
- Clean GUI: essential stats (FPS, body count, zoom) and controls
- Shader and rendering robustness improvements (point-size clamping, stable background shader hash, gamma correction).

## Repository Layout

- `src/` - source files and core implementation.
  - `core/Camera.cpp`, `core/Renderer.cpp`, `core/GUI.cpp`, `core/Simulation.cpp`, `glad.c`, `main.cpp`
- `include/` - headers for the project (planets/*.hpp, glad headers, etc.)
- `assets/` - images and other assets. Add screenshots under `assets/screenshots/`.
- `build.bat`, `start.bat`, `CMakeLists.txt` - build and run scripts.

## Build (Windows - PowerShell)

1. Open PowerShell and navigate to the project root:

2. Build the project (this uses the included `build.bat` which wraps CMake + mingw32-make):

```powershell
.\build.bat
```

3. Run the application (use the included `start.bat` or execute the binary directly):

```powershell
.\start.bat
# or
./build/PlanetsProject.exe
```

## Quick Usage

- Mouse scroll: zoom
- `+` / `-` buttons in the GUI: zoom in/out (these now apply a persistent zoom offset)
- Double-click a planet in the simulation pane: camera follows that planet
- Double-click the same planet again: camera returns to following the system COM
- `H`: toggle GUI panel
- Pause/Play and time scale controls are available in the GUI panel

## License

This project inherits licenses from included third-party libraries (see `lib/` and their LICENSE files). The project code in this repository is provided under the MIT license.