import os
import glob
import argparse
import sysconfig
import sys

# === Argument Parsing ===
parser = argparse.ArgumentParser(description="Build system for static library, test executable, and Python module.")
parser.add_argument("--mode", choices=["debug", "release"], default="debug", help="Build mode (default: debug)")
parser.add_argument("--no-pybindings", action="store_true", help="Skip building Python bindings")
args = parser.parse_args()
mode = args.mode.lower()
build_pybindings = not args.no_pybindings

# === Directories / Names ===
root_dir = os.path.abspath(".")
src_dir = os.path.join(root_dir, "src")
bindings_dir = os.path.join(src_dir, "bindings")
build_dir = os.path.join(root_dir, f"build/{mode}")
lib_name = "debuglib.lib"
test_file = os.path.join(root_dir, "test.cpp")
exe_name = "test.exe"
pybind_module = "dbg.pyd"

# === Compiler Flags ===
if mode == "debug":
    common_flags = "/EHsc /Zi /std:c++20 /Od"
else:
    common_flags = "/EHsc /std:c++20 /O2 /DNDEBUG"

# === Include Paths ===
# Priority: PYBIND11_INCLUDE -> PYBIND11_DIR/include -> extern/pybind11/include
pybind_include = (
    os.environ.get("PYBIND11_INCLUDE")
    or (os.path.join(os.environ.get("PYBIND11_DIR", ""), "include") if os.environ.get("PYBIND11_DIR") else None)
    or os.path.join(root_dir, "extern", "pybind11", "include")
)
pybind_include = os.path.abspath(pybind_include)

python_include = sysconfig.get_path("include")
include_flags = ' '.join([
    f'/I"{pybind_include}"',
    f'/I"{python_include}"',
    f'/I"{src_dir}"',
    f'/I"{bindings_dir}"'
])

# Sanity check for pybind11
if not os.path.isdir(pybind_include) or not os.path.isfile(os.path.join(pybind_include, "pybind11", "pybind11.h")):
    print(f"[!] pybind11 not found at: {pybind_include}")
    print("    Set PYBIND11_INCLUDE to your pybind11/include directory, e.g.:")
    print(r'    set PYBIND11_INCLUDE=extern\pybind11\include')
    sys.exit(1)

# === Ensure Build Directory Exists ===
os.makedirs(build_dir, exist_ok=True)

# === Step 1: Compile .cpp in src (top-level only) to .obj ===
cpp_files = glob.glob(os.path.join(src_dir, "*.cpp"))
obj_files = []
for cpp in cpp_files:
    obj = os.path.join(build_dir, os.path.splitext(os.path.basename(cpp))[0] + ".obj")
    cmd = f'cl /c {common_flags} {include_flags} /Fo"{obj}" /Fd"{build_dir}\\vc140.pdb" "{cpp}"'
    print(f"[+] Compiling {os.path.relpath(cpp, root_dir)}")
    if os.system(cmd) != 0:
        sys.exit(1)
    obj_files.append(obj)

# === Step 2: Create .lib ===
lib_path = os.path.join(build_dir, lib_name)
if obj_files:
    obj_str = " ".join(f'"{obj}"' for obj in obj_files)
    cmd = f'lib /OUT:"{lib_path}" {obj_str}'
    print(f"[+] Creating static library {os.path.relpath(lib_path, root_dir)}")
    if os.system(cmd) != 0:
        sys.exit(1)
else:
    print("[~] No objects from src/*.cpp, skipping static library")
    lib_path = None

# === Step 3: Optionally compile test.cpp and link ===
if os.path.isfile(test_file):
    test_obj = os.path.join(build_dir, "test.obj")
    exe_path = os.path.join(build_dir, exe_name)

    cmd = f'cl /c {common_flags} {include_flags} /Fo"{test_obj}" /Fd"{build_dir}\\vc140.pdb" "{test_file}"'
    print(f"[+] Compiling {os.path.relpath(test_file, root_dir)}")
    if os.system(cmd) != 0:
        sys.exit(1)

    link_inputs = f'"{test_obj}"'
    if lib_path:
        link_inputs += f' "{lib_path}"'

    cmd = f'cl {common_flags} /Fe"{exe_path}" {link_inputs}'
    print(f"[+] Linking {os.path.relpath(exe_path, root_dir)}")
    if os.system(cmd) != 0:
        sys.exit(1)
else:
    print("[~] test.cpp not found; skipping test.exe")

# === Step 4: Build pybind11 module (if not skipped and sources exist) ===
if build_pybindings:
    pybind_files = glob.glob(os.path.join(bindings_dir, "*.cpp"))
    if not pybind_files:
        print("[~] No binding sources in src/bindings; skipping Python module")
    else:
        pybind_objs = []
        python_version = f"{sys.version_info.major}{sys.version_info.minor}"
        python_lib = f"python{python_version}.lib"
        python_base = sysconfig.get_config_var("base")
        python_libdir = sysconfig.get_config_var("LIBDIR") or os.path.join(python_base, "libs")

        for pybind_cpp in pybind_files:
            pybind_obj = os.path.join(build_dir, os.path.splitext(os.path.basename(pybind_cpp))[0] + ".obj")
            cmd = f'cl /c {common_flags} {include_flags} /Fo"{pybind_obj}" /Fd"{build_dir}\\vc140.pdb" "{pybind_cpp}"'
            print(f"[+] Compiling {os.path.relpath(pybind_cpp, root_dir)}")
            if os.system(cmd) != 0:
                sys.exit(1)
            pybind_objs.append(pybind_obj)

        pybind_pyd = os.path.join(build_dir, pybind_module)
        obj_str = " ".join(f'"{obj}"' for obj in pybind_objs)

        lib_arg = f'"{lib_path}"' if lib_path else ""
        cmd = (
            f'link /DLL /OUT:"{pybind_pyd}" {obj_str} {lib_arg} '
            f'/LIBPATH:"{python_libdir}" {python_lib} /EXPORT:PyInit_dbg'
        )
        print(f"[+] Linking Python module {os.path.relpath(pybind_pyd, root_dir)}")
        if os.system(cmd) != 0:
            sys.exit(1)
else:
    print("[~] Skipping Python bindings (--no-pybindings specified)")
