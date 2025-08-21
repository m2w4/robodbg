::Hacky Helper Script to build the wheel
cd ..
rmdir /s /q build

cmake -S . -B build -DROBO_BUILD_PYTHON=ON -DROBO_BUILD_EXAMPLES=ON -DROBO_BUILD_TESTS=ON -DROBO_BUILD_DOCS=ON -DPython_EXECUTABLE="C:\Users\user\AppData\Local\Programs\Python\Python311\python.exe"
cmake --build build --config Release
copy build\python\Release\dbg.*.pyd .\wheel\src\robodbg\
del build\CMakeCache.txt

cd wheel
"C:\Users\user\AppData\Local\Programs\Python\Python311\python.exe" -m build
cd ..
del .\wheel\src\robodbg\*.pyd
del .\build\python\Release\dbg.*.pyd

cmake -S . -B build -DROBO_BUILD_PYTHON=ON -DROBO_BUILD_EXAMPLES=ON -DROBO_BUILD_TESTS=ON -DROBO_BUILD_DOCS=ON -DPython_EXECUTABLE="C:\Users\user\AppData\Local\Programs\Python\Python312\python.exe"
cmake --build build --config Release
copy build\python\Release\dbg.*.pyd .\wheel\src\robodbg\
del build\CMakeCache.txt

cd wheel
"C:\Users\user\AppData\Local\Programs\Python\Python312\python.exe" -m build
cd ..
del .\wheel\src\robodbg\*.pyd
del .\build\python\Release\dbg.*.pyd

cmake -S . -B build -DROBO_BUILD_PYTHON=ON -DROBO_BUILD_EXAMPLES=ON -DROBO_BUILD_TESTS=ON -DROBO_BUILD_DOCS=ON -DPython_EXECUTABLE="C:\Users\user\AppData\Local\Programs\Python\Python313\python.exe"
cmake --build build --config Release
copy build\python\Release\dbg.*.pyd .\wheel\src\robodbg\
del build\CMakeCache.txt

cd wheel
"C:\Users\user\AppData\Local\Programs\Python\Python313\python.exe" -m build
cd ..
del .\wheel\src\robodbg\*.pyd
del .\build\python\Release\dbg.*.pyd

cd wheel
