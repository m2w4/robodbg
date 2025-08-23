# ---------- Python bindings (dbg.pyd via nanobind) ----------
if(NOT BUILD_PYTHON)
  return()
endif()

# Ensure CMake defines Python::Module / Python::Interpreter for nanobind
# (Use the 'Python' package, not 'Python3', with these components.)
find_package(Python 3.11...3.13.13  REQUIRED
  COMPONENTS Interpreter Development.Module Development)

set(BINDINGS_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Path to Python bindings directory")

file(GLOB_RECURSE ROBODBG_BIND_SOURCES CONFIGURE_DEPENDS
  "${BINDINGS_DIR}/*.cpp" "${BINDINGS_DIR}/*.cc" "${BINDINGS_DIR}/*.cxx"
)

if(NOT ROBODBG_BIND_SOURCES)
  message(FATAL_ERROR "No Python binding sources found in ${BINDINGS_DIR}")
endif()

# Prefer vendored nanobind if present; otherwise use an installed package
if(EXISTS "${CMAKE_SOURCE_DIR}/extern/nanobind/CMakeLists.txt")
  message(STATUS "Using vendored nanobind at extern/nanobind")
  # When the source dir is outside this CMakeLists, specify a binary dir.
  set(NANOBIND_BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/nanobind-build")
  set(NANOBIND_STATIC ON CACHE BOOL "" FORCE)
  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/extern/nanobind"
    "${NANOBIND_BINARY_DIR}"
    EXCLUDE_FROM_ALL
  )
else()
  find_package(nanobind CONFIG REQUIRED) # provides nanobind_add_module()
endif()

# Build the Python extension module named 'dbg' (matches NB_MODULE(dbg, m))
nanobind_add_module(dbg ${ROBODBG_BIND_SOURCES})

nanobind_add_stub(
  my_ext_stub
  MODULE dbg
  OUTPUT dbg.pyi
  PYTHON_PATH $<TARGET_FILE_DIR:dbg>
  MARKER_FILE py.typed         # creates py.typed for PEP 561
  DEPENDS dbg
)

install(TARGETS dbg DESTINATION ".")
install(FILES py.typed dbg DESTINATION ".")

target_link_libraries(dbg PRIVATE robodbg_core)
target_compile_features(dbg PRIVATE cxx_std_20)
target_include_directories(dbg PRIVATE "${CMAKE_SOURCE_DIR}/src")

set_target_properties(dbg PROPERTIES OUTPUT_NAME "dbg"
  PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python/$<CONFIG>"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python/$<CONFIG>"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python/$<CONFIG>"
)

add_custom_target(show_dbg_path ALL
  COMMAND ${CMAKE_COMMAND} -E echo "dbg extension => $<TARGET_FILE:dbg>"
  DEPENDS dbg
  VERBATIM
)
