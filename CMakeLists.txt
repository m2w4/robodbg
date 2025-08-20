cmake_minimum_required(VERSION 3.20)

project(RoboDBG
  VERSION 0.0.2
  DESCRIPTION "Windows user-mode debugger core + Python bindings"
  LANGUAGES CXX C)

# ---------- Options ----------
option(BUILD_PYTHON "Build Python extension module" ON)

# ---------- Global settings ----------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories (bin/lib) per-config
foreach(dir RUNTIME LIBRARY ARCHIVE PDB)
  set(CMAKE_${dir}_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endforeach()

# Windows-specific sane defaults
if(MSVC)
  add_compile_options(/W4 /permissive- /Zc:preprocessor /Zc:__cplusplus /EHsc /bigobj)
  add_compile_definitions(
    _CRT_SECURE_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
  )
endif()

# ---------- Core library ----------
add_subdirectory(src)

# ---------- Python bindings (dbg.pyd) ----------
if(BUILD_PYTHON)
  add_subdirectory(bindings)
endif()


