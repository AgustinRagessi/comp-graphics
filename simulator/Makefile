# ==============================================================================
# FLIGHT SIMULATOR BUILD SCRIPT (MAKEFILE) [cite: 1]
# 
# This file defines how the C++ compiler (g++) should assemble the source code 
# into a final executable. It automates compilation by defining the compiler flags, 
# include paths, linking external libraries, and tracking project dependencies.
# ==============================================================================

# ---------------------------------------------------------
# COMPILER SETUP
# ---------------------------------------------------------
# CXX: Defines the compiler to use (GNU C++ Compiler).
CXX = g++

# CXXFLAGS: Settings passed to the compiler during the build phase.
#  -std=c++17 : Enforces the C++17 standard language features.
#  -Wall      : (Warnings All) Enables all standard compiler warnings to catch potential bugs early.
#  -O2        : Applies Level-2 optimizations for execution performance (crucial for game loops).
CXXFLAGS = -std=c++17 -Wall -O2

# TARGET: The name of the final output executable file.
TARGET = flight-simulator

# ---------------------------------------------------------
# DEPENDENCIES AND PATHS
# ---------------------------------------------------------
# INC_FLAGS (Include Flags): 
# Tells the compiler where to look for header (.h) files when it encounters an #include statement.
#  -I.            : Includes the current root directory.
#  -I../...       : Links local dependencies like GLAD (OpenGL loader) and STB (image loading).
#  -I/mingw64/... : Links global compiler library paths. [cite: 2]
INC_FLAGS = -I. -I../glad/include -I../stb -I../CGyAV-dlfdm/include -I/mingw64/include

# LDFLAGS (Linker Flags):
# Tells the linker which compiled libraries to merge into the final executable.
#  -L/mingw64/lib : Directory to search for libraries.
#  -lglfw3        : Links the GLFW library (Window creation/Input).
#  -lopengl32     : Links the core Windows OpenGL library.
#  -lgdi32        : Links the Windows Graphics Device Interface (required by GLFW).
LDFLAGS = -L/mingw64/lib -lglfw3 -lopengl32 -lgdi32

# ---------------------------------------------------------
# SOURCE FILES
# ---------------------------------------------------------
# Explicitly lists all .cpp and .c files required to build the program. 
# Grouping them by module keeps the Makefile organized and maintainable.
MAIN_SRC = main/main.cpp
WINDOW_SRC = opengl/window.cpp
SKYBOX_SRC = map/skybox.cpp
TERRAIN_SRC = map/terrain.cpp
AIRPLANE_SRC = airplane/airplane.cpp
FDM_AERO_SRC = ../CGyAV-dlfdm/src/dlfdm/aerodynamicsmodel.cpp
FDM_DYN_SRC = ../CGyAV-dlfdm/src/dlfdm/aircraftdynamics.cpp
FDM_SOLVER_SRC = ../CGyAV-dlfdm/src/dlfdm/fdmsolver.cpp
STB_SRC = stb_impl.cpp
GLAD_SRC = ../glad/src/glad.c
CAMERA_SRC = camera/camera.cpp
HUD_SRC = hud/hud_manager.cpp hud/variometer.cpp hud/altimeter.cpp hud/airspeed.cpp hud/compass.cpp hud/horizon.cpp hud/waypointTracker.cpp
WAYPOINT_SRC = waypoint/waypoint.cpp

# ALL_SRC: A single variable consolidating all sources for the build rule.
ALL_SRC = $(MAIN_SRC) $(WINDOW_SRC) $(SKYBOX_SRC) $(TERRAIN_SRC) $(AIRPLANE_SRC) $(FDM_AERO_SRC) $(FDM_DYN_SRC) $(FDM_SOLVER_SRC) $(STB_SRC) $(GLAD_SRC) $(CAMERA_SRC) $(HUD_SRC) $(WAYPOINT_SRC)

# ---------------------------------------------------------
# BUILD RULES
# ---------------------------------------------------------
# Rule: $(TARGET).exe
# Depends on: $(ALL_SRC)
# Action: Executes the g++ compiler with all the flags defined above.
$(TARGET).exe: $(ALL_SRC)
	@echo "Compiling Flight Simulator..."
	@$(CXX) $(CXXFLAGS) $(INC_FLAGS) $(ALL_SRC) -o $(TARGET).exe $(LDFLAGS)
	@echo "Build complete: $(TARGET).exe"

# .PHONY tells Make that 'run' and 'clean' are commands, not actual files to build.
.PHONY: run clean

# Rule: run
# Action: Compiles (if necessary) and immediately launches the simulator.
run: $(TARGET).exe
	@echo "Running Flight Simulator..."
	@./$(TARGET).exe

# Rule: clean
# Action: Deletes the compiled executable to force a fresh build on the next run.
clean:
	@echo "Cleaning..."
	@rm -f $(TARGET).exe