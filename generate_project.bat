set current=%~dp0
mkdir project
cd project
cmake ../ -A x64 -T "ClangCL" -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static-md
cd %~dp0
