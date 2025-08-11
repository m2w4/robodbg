cmake_minimum_required(VERSION 3.23)

project(robodbg
  VERSION 0.1.0
  LANGUAGES CXX C)

# ---- Options ---------------------------------------------------------------

option(BUILD_SHARED_LIBS "Build shared libraries instead of static" OFF)
option(ROBO_BUILD_PYTHON "Build Python extension (pybind11)" ON)
# option(ROBO_BUILD_EXAMPLES "Build examples" ON)
# option(ROBO_BUILD_TESTS "Build tests" ON)
option(ROBO_BUILD_DOCS "Enable Doxygen docs target" ON)

# ---- Global settings -------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# MSVC niceties
if(MSVC)
  add_compile_options(/W4 /permissive- /Zc:__cplusplus /EHsc)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Windows lean headers
add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)

# ---- Dependencies ----------------------------------------------------------

# pybind11: prefer submodule at extern/pybind11 but allow system install
set(ROBO_PYBIND11_FROM_SUBMODULE FALSE)
if(ROBO_BUILD_PYTHON)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/pybind11/CMakeLists.txt")
    message(STATUS "Using pybind11 from submodule: extern/pybind11")
    set(ROBO_PYBIND11_FROM_SUBMODULE TRUE)
    add_subdirectory(extern/pybind11)
  else()
    find_package(pybind11 CONFIG REQUIRED)  # provides pybind11::module
  endif()
endif()

# ---- Subdirectories --------------------------------------------------------

add_subdirectory(src)

if(ROBO_BUILD_PYTHON)
  add_subdirectory(bindings)
endif()

# if(ROBO_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
#   add_subdirectory(examples)
# endif()

#if(ROBO_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
#  enable_testing()
#  add_subdirectory(tests)
#endif()

# ---- Doxygen (optional) ----------------------------------------------------

if(ROBO_BUILD_DOCS)
  find_package(Doxygen QUIET)
  if(DOXYGEN_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile")
    add_custom_target(docs
      COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)
  else()
    message(STATUS "Doxygen or Doxyfile not found; 'docs' target disabled")
  endif()
endif()

# ---- Install & CMake package export ----------------------------------------

include(GNUInstallDirs)

install(
  TARGETS robodbg
  EXPORT robodbgTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Public headers (adjust if you later move to include/robodbg/)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/robodbg
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

# Generate and install a CMake package config
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/robodbgConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/robodbgConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/robodbgConfig.cmake"
  @ONLY
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/robodbgConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/robodbgConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/robodbg
)

install(EXPORT robodbgTargets
  NAMESPACE robodbg::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/robodbg
)