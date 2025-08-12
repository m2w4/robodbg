cmake_minimum_required(VERSION 3.20)

project(RoboDBG
  VERSION 0.1.0
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

# ---------- Python bindings (robodbg.pyd) ----------
if(BUILD_PYTHON)
  # Prefer vendored pybind11 if present; fallback to find_package
  if(EXISTS "${CMAKE_SOURCE_DIR}/extern/pybind11/CMakeLists.txt")
    message(STATUS "Using vendored pybind11 at extern/pybind11")
    add_subdirectory(extern/pybind11)
  else()
    find_package(pybind11 CONFIG REQUIRED)
  endif()

  file(GLOB_RECURSE ROBODBG_BIND_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/bindings/*.cpp"
    "${CMAKE_SOURCE_DIR}/bindings/*.cc"
    "${CMAKE_SOURCE_DIR}/bindings/*.cxx"
  )

  if(ROBODBG_BIND_SOURCES STREQUAL "")
    message(FATAL_ERROR "No binding sources found in src/bindings/")
  endif()

  # Build the Python extension
  pybind11_add_module(robodbg ${ROBODBG_BIND_SOURCES})

  target_link_libraries(robodbg PRIVATE robodbg_core)

  target_compile_features(robodbg PRIVATE cxx_std_20)
  target_include_directories(robodbg PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
  )

  # Force bare name & correct extension; no SOABI/arch suffixes
  set_target_properties(robodbg PROPERTIES
    PREFIX ""                   # no 'lib' prefix
    OUTPUT_NAME "robodbg"       # exact name
    SUFFIX ".pyd"               # ensure .pyd on Windows
    # Put the extension in a predictable folder:
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python/$<CONFIG>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python/$<CONFIG>"
  )

  # If anything needs the file path later, use $<TARGET_FILE:robodbg>
  add_custom_target(show_robodbg_path ALL
    COMMAND ${CMAKE_COMMAND} -E echo "robodbg.pyd => $<TARGET_FILE:robodbg>"
    DEPENDS robodbg
    VERBATIM
  )
endif()


