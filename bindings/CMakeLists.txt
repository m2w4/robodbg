# Python extension: name will be 'robodbg' (import robodbg)
set(ROBO_PY_SRC "${CMAKE_CURRENT_SOURCE_DIR}/robodbg.pybind.cpp")
if(NOT EXISTS "${ROBO_PY_SRC}")
  message(FATAL_ERROR "bindings/robodbg.pybind.cpp not found. Adjust path if needed.")
endif()

# Use pybind11’s helper
pybind11_add_module(robodbg_py MODULE "${ROBO_PY_SRC}")

# Match import name to 'robodbg' (Windows .pyd name)
set_target_properties(robodbg_py PROPERTIES OUTPUT_NAME "robodbg")

target_link_libraries(robodbg_py PRIVATE robodbg)

target_include_directories(robodbg_py PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Ensure the module ends up next to the library binaries during build
set_target_properties(robodbg_py PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python"
)
