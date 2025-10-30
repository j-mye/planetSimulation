# Minimum CMake version
cmake_minimum_required(VERSION 3.15)

# Project setup
project(PlanetsProject LANGUAGES C CXX)

# Enforce C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prefer config package provided by MSYS2/vcpkg for GLFW
find_package(glfw3 CONFIG REQUIRED)

# Collect all source files
file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# -------------------------------------------------------------
# ImGui setup
# -------------------------------------------------------------
# Add ImGui source files manually (these are not compiled automatically)
set(IMGUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/backends/imgui_impl_opengl3.cpp
)

list(APPEND SOURCES ${IMGUI_SOURCES})

# -------------------------------------------------------------
# Define the executable target
# -------------------------------------------------------------
# WIN32 flag removes console window on Windows
add_executable(PlanetsProject WIN32 ${SOURCES})

# -------------------------------------------------------------
# Include directories
# -------------------------------------------------------------
target_include_directories(PlanetsProject PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include               # My headers
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/glm               # GLM math library
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui             # ImGui core headers
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/backends    # ImGui backend headers
)

# -------------------------------------------------------------
# Link required libraries
# -------------------------------------------------------------
# Link GLFW target name varies by package; support both
if (TARGET glfw)
    target_link_libraries(PlanetsProject PRIVATE glfw)
elseif (TARGET glfw3)
    target_link_libraries(PlanetsProject PRIVATE glfw3)
else()
    message(FATAL_ERROR "glfw or glfw3 target not found from find_package(glfw3)")
endif()

target_link_libraries(PlanetsProject PRIVATE
    opengl32     # OpenGL
    gdi32        # Windows GDI
    user32       # Windows window/input
    kernel32     # Base system functions
    winmm        # Multimedia timers
)

# -------------------------------------------------------------
# Compiler definitions
# -------------------------------------------------------------
target_compile_definitions(PlanetsProject PRIVATE GLFW_DLL)

# -------------------------------------------------------------
# Output settings
# -------------------------------------------------------------
set_target_properties(PlanetsProject PROPERTIES
    OUTPUT_NAME "PlanetsProject"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
