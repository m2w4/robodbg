# Library sources. For now we use GLOB to avoid guessing your file list.
# Replace with explicit lists once stable.
file(GLOB_RECURSE ROBO_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

file(GLOB_RECURSE ROBO_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

add_library(robodbg ${ROBO_SOURCES} ${ROBO_HEADERS})

target_include_directories(robodbg
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/robodbg>
)

# Windows libs (present in repo headers like <winternl.h>, Psapi usage, etc.)
# Link only if available on the platform.
if(WIN32)
  target_link_libraries(robodbg
    PUBLIC
      advapi32
      ntdll
      psapi
  )
endif()

# target_compile_definitions(robodbg PUBLIC UNICODE _UNICODE) TODO: switch to Unicode
