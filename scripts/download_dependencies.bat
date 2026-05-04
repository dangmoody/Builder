@echo off

setlocal EnableDelayedExpansion

pushd %~dp0

:: ----------------------------------------------------------------

:: download clang
set clangVersion=20.1.5

if not exist ..\\clang mkdir ..\\clang

echo Downloading Clang archive...
curl -o ..\\clang\\clang.tar.xz -L "https://github.com/llvm/llvm-project/releases/download/llvmorg-%clangVersion%/clang+llvm-%clangVersion%-x86_64-pc-windows-msvc.tar.xz"
echo Done.
echo.

:: unarchive clang
echo Extracing Clang archive...
..\\tools\\7zip_win64\\7za.exe x -y -aoa "..\\clang\\clang.tar.xz" -o"..\\clang"
..\\tools\\7zip_win64\\7za.exe x -y -aoa "..\\clang\\clang.tar" -o"..\\clang"
robocopy /nfl /ndl /e /is "..\\clang\\clang+llvm-%clangVersion%-x86_64-pc-windows-msvc" "..\\clang"
rmdir /s /q "..\\clang\\clang+llvm-%clangVersion%-x86_64-pc-windows-msvc"
del /q "..\\clang\\clang.tar"
del /q "..\\clang\\clang.tar.xz"
echo Done.
echo.

:: Remove unused Clang files to reduce size
echo Removing unused Clang files...

:: Entire top-level directories not needed at runtime
if exist "..\\clang\\include"  rmdir /s /q "..\\clang\\include"
if exist "..\\clang\\share"    rmdir /s /q "..\\clang\\share"

:: Subdirectories of lib/ not needed at runtime
:: (lib\clang\ is the compiler resource dir and must be kept)
if exist "..\\clang\\lib\\cmake"        rmdir /s /q "..\\clang\\lib\\cmake"
if exist "..\\clang\\lib\\libscanbuild" rmdir /s /q "..\\clang\\lib\\libscanbuild"
if exist "..\\clang\\lib\\libear"       rmdir /s /q "..\\clang\\lib\\libear"

:: From lib\: delete all .lib files (lib\clang\ subdirectory is unaffected — no /s flag)
del /q "..\\clang\\lib\\*.lib" 2>nul

:: From bin\: delete everything except the tools Builder needs
for %%f in ("..\\clang\\bin\\*") do (
    set "keep="
    if /i "%%~nxf"=="clang.exe"         set "keep=1"
    if /i "%%~nxf"=="clang++.exe"       set "keep=1"
    if /i "%%~nxf"=="clang-20.exe"      set "keep=1"
    if /i "%%~nxf"=="clang-cl.exe"      set "keep=1"
    if /i "%%~nxf"=="clang-cpp.exe"     set "keep=1"
    if /i "%%~nxf"=="lld.exe"           set "keep=1"
    if /i "%%~nxf"=="lld-link.exe"      set "keep=1"
    if /i "%%~nxf"=="ld.lld.exe"        set "keep=1"
    if /i "%%~nxf"=="llvm-ar.exe"       set "keep=1"
    if /i "%%~nxf"=="llvm-ranlib.exe"   set "keep=1"
    if /i "%%~nxf"=="llvm-lib.exe"      set "keep=1"
    if /i "%%~nxf"=="llvm-dlltool.exe"  set "keep=1"
    if /i "%%~nxf"=="libclang.dll"      set "keep=1"
    if /i "%%~nxf"=="LLVM-C.dll"        set "keep=1"
    if /i "%%~nxf"=="LTO.dll"           set "keep=1"
    if /i "%%~nxf"=="Remarks.dll"       set "keep=1"
    if not defined keep del /q "%%f"
)

echo Done.
echo.

:: ----------------------------------------------------------------

set gccVersion=15.1.0
if not exist ..\\tools\\gcc mkdir ..\\tools\\gcc

:: download gcc
echo Downloading GCC archive...
curl -o ..\\tools\\gcc\\gcc.7z -L "https://github.com/brechtsanders/winlibs_mingw/releases/download/%gccVersion%posix-12.0.0-ucrt-r1/winlibs-x86_64-posix-seh-gcc-%gccVersion%-mingw-w64ucrt-12.0.0-r1.7z"
echo Done.
echo.

:: unarchive gcc
echo Extracing GCC archive...
..\\tools\\7zip_win64\\7za.exe x -y -aoa "..\\tools\\gcc\\gcc.7z" -o"..\\tools\\gcc"
robocopy /nfl /ndl /e "..\\tools\\gcc\\mingw64" "..\\tools\\gcc"
del /q "..\\tools\\gcc\\gcc.7z"
echo Done.
echo.

:: ----------------------------------------------------------------

echo Done!

popd