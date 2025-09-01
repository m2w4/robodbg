# Core library collecting all non-binding sources.

# Grab all core sources (exclude bindings/)
file(GLOB_RECURSE ROBODBG_CORE_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/plugins/*.cpp"
  "${CMAKE_SOURCE_DIR}/src/plugins/*.cc"
  "${CMAKE_SOURCE_DIR}/src/plugins/*.cxx"
)
# Remove binding sources from the core list (they are compiled into the .pyd)
list(FILTER ROBODBG_CORE_SOURCES EXCLUDE REGEX "/bindings/")

if(ROBODBG_CORE_SOURCES STREQUAL "")
  message(FATAL_ERROR "No core sources found in src/ (excluding src/bindings).")
endif()

add_library(robodbg_core STATIC ${ROBODBG_CORE_SOURCES})
add_library(RoboDBG::core ALIAS robodbg_core)

target_compile_features(robodbg_core PUBLIC cxx_std_20)
target_include_directories(robodbg_core
  PUBLIC
    "${CMAKE_SOURCE_DIR}/src"        # public headers live in src/ for now
)

# Link Windows libs used by the debugger core
if(WIN32)
  target_link_libraries(robodbg_core
    PUBLIC
      Advapi32
      Psapi
      ntdll
  )
endif()

# Keep core outputs alongside top-level outputs
set_target_properties(robodbg_core PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/$<CONFIG>"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/$<CONFIG>"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/$<CONFIG>"
)
