# ---------- Python bindings (robodbg.pyd) ----------
if(BUILD_PYTHON)
  # Default: look in top-level /bindings
  set(BINDINGS_DIR "${CMAKE_SOURCE_DIR}/bindings" CACHE PATH "Path to Python bindings directory")

  file(GLOB_RECURSE ROBODBG_BIND_SOURCES CONFIGURE_DEPENDS
    "${BINDINGS_DIR}/*.cpp" "${BINDINGS_DIR}/*.cc" "${BINDINGS_DIR}/*.cxx"
  )

  if(NOT ROBODBG_BIND_SOURCES)
    message(WARNING "No Python binding sources found in ${BINDINGS_DIR}. Disabling BUILD_PYTHON.")
    set(BUILD_PYTHON OFF)
  endif()
endif()

if(BUILD_PYTHON)
  if(EXISTS "${CMAKE_SOURCE_DIR}/extern/pybind11/CMakeLists.txt")
    message(STATUS "Using vendored pybind11 at extern/pybind11")
    add_subdirectory(extern/pybind11)
  else()
    find_package(pybind11 CONFIG REQUIRED)
  endif()

  pybind11_add_module(robodbg ${ROBODBG_BIND_SOURCES})

  target_link_libraries(robodbg PRIVATE robodbg_core)
  target_compile_features(robodbg PRIVATE cxx_std_20)
  target_include_directories(robodbg PRIVATE "${CMAKE_SOURCE_DIR}/src")

  set_target_properties(robodbg PROPERTIES
    PREFIX ""
    OUTPUT_NAME "robodbg"
    SUFFIX ".pyd"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python/$<CONFIG>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python/$<CONFIG>"
  )
endif()
