@echo off
setlocal

REM ---- Settings you can tweak ----
set "PROJECT=RoboDBG"
set "INPUT=src"
set "OUTPUT=docs"

REM ---- Generate minimal, clean Doxyfile focusing only on src/ ----
> Doxyfile (
  echo PROJECT_NAME=%PROJECT%
  echo OUTPUT_DIRECTORY=%OUTPUT%
  echo GENERATE_HTML=YES
  echo GENERATE_LATEX=NO
  echo RECURSIVE=YES
  echo INPUT=%INPUT%
  echo FILE_PATTERNS=*.h *.hpp *.hh *.c *.cc *.cpp
  echo FULL_PATH_NAMES=NO
  echo STRIP_FROM_PATH=.
  echo EXTRACT_ALL=NO
  echo EXTRACT_PRIVATE=NO
  echo EXTRACT_PROTECTED=YES
  echo EXTRACT_STATIC=YES
  echo EXTRACT_LOCAL_METHODS=NO
  echo EXTRACT_ANON_NSPACES=NO
  echo HIDE_UNDOC_MEMBERS=YES
  echo HIDE_UNDOC_CLASSES=YES
  echo WARN_IF_UNDOCUMENTED=NO
  echo GENERATE_TREEVIEW=YES
  echo QUIET=NO

  REM Only care about your own source: exclude bindings, externals, tests, build outputs, etc.
  echo EXCLUDE= ^
    src\\bindings ^
    extern ^
    third_party ^
    tests ^
    build ^
    out ^
    .git

  REM Extra safety so pybind11 noise (if any slips in) is ignored
  echo EXCLUDE_PATTERNS= */bindings/* */extern/* */third_party/* */tests/* */build/* */out/*

  REM Keep preprocessing simple; ignore pybind11 macros if headers get seen indirectly
  echo ENABLE_PREPROCESSING=YES
  echo MACRO_EXPANSION=NO
  echo EXPAND_ONLY_PREDEF=YES
  REM Nice source browser in HTML
  echo SOURCE_BROWSER=YES
  echo INLINE_SOURCES=NO
  echo REFERENCED_BY_RELATION=YES
  echo REFERENCES_RELATION=YES
)

if not exist "%OUTPUT%" mkdir "%OUTPUT%" >nul 2>&1

where doxygen >nul 2>&1
if errorlevel 1 (
  echo.
  echo [!] Doxygen is not in PATH. Install it and ensure 'doxygen.exe' is available.
  echo     Download: https://www.doxygen.nl/download.html
  exit /b 1
)

echo [*] Running doxygen...
doxygen Doxyfile
if errorlevel 1 (
  echo [!] Doxygen failed.
  exit /b 1
)

echo [✓] HTML docs generated at: %OUTPUT%\html\index.html
endlocal
