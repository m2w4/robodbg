::Hacky Helper Script to build the wheel
cd ..
rmdir /s /q build-win32

cmake -S . -B build-win32 -A Win32 -DBUILD_PYTHON=ON -DPython_EXECUTABLE="C:\Users\user\AppData\Local\Programs\Python\Python311-32\python.exe"
cmake --build build-win32 --config Release
copy build-win32\python\Release\dbg.*.pyd .\wheel\src\robodbg\
del build-win32\CMakeCache.txt

cd wheel
"C:\Users\user\AppData\Local\Programs\Python\Python311-32\python.exe" -m build
cd ..
del .\wheel\src\robodbg\*.pyd
del .\build-win32\python\Release\dbg.*.pyd


cmake -S . -B build-win32 -A Win32 -DBUILD_PYTHON=ON -DPython_EXECUTABLE="C:\Users\user\AppData\Local\Programs\Python\Python312-32\python.exe"
cmake --build build-win32 --config Release
copy build-win32\python\Release\dbg.*.pyd .\wheel\src\robodbg\
del build-win32\CMakeCache.txt

cd wheel
"C:\Users\user\AppData\Local\Programs\Python\Python312-32\python.exe" -m build
cd ..
del .\wheel\src\robodbg\*.pyd
del .\build-win32\python\Release\dbg.*.pyd


cmake -S . -B build-win32 -A Win32 -DBUILD_PYTHON=ON -DPython_EXECUTABLE="C:\Users\user\AppData\Local\Programs\Python\Python313-32\python.exe"
cmake --build build-win32 --config Release
copy build-win32\python\Release\dbg.*.pyd .\wheel\src\robodbg\
del build-win32\CMakeCache.txt

cd wheel
"C:\Users\user\AppData\Local\Programs\Python\Python313-32\python.exe" -m build
cd ..
del .\wheel\src\robodbg\*.pyd
del .\build-win32\python\Release\dbg.*.pyd

cd wheel
