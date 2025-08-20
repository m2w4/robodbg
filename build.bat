cmake -S . -B build -DROBO_BUILD_PYTHON=ON -DROBO_BUILD_EXAMPLES=ON -DROBO_BUILD_TESTS=ON -DROBO_BUILD_DOCS=ON

:: Build (choose Release for distribution)
cmake --build build --config Release
