@echo off

setlocal EnableDelayedExpansion

pushd %~dp0

:: ----------------------------------------------------------------

:: download clang
set clangVersion=20.1.5

if not exist ..\\tools\\clang (
	mkdir ..\\tools\\clang
)

echo Downloading Clang...
curl -o ..\\tools\\clang\\clang.tar.xz -L "https://github.com/llvm/llvm-project/releases/download/llvmorg-%clangVersion%/clang+llvm-%clangVersion%-x86_64-pc-windows-msvc.tar.xz"
echo Done.
echo.

:: unarchive clang
echo Extracting Clang...
..\\tools\\7zip-win64\\7za.exe x -y -aoa "..\\tools\\clang\\clang.tar.xz" -o"..\\tools\\clang"
..\\tools\\7zip-win64\\7za.exe x -y -aoa "..\\tools\\clang\\clang.tar" -o"..\\tools\\clang"
robocopy /nfl /ndl /e /is "..\\tools\\clang\\clang+llvm-%clangVersion%-x86_64-pc-windows-msvc" "..\\tools\\clang"
rmdir /s /q "..\\tools\\clang\\clang+llvm-%clangVersion%-x86_64-pc-windows-msvc"
del /q "..\\tools\\clang\\clang.tar"
del /q "..\\tools\\clang\\clang.tar.xz"
echo Done.
echo.

:: ----------------------------------------------------------------

:: download gcc
set gccVersion=15.1.0
if not exist ..\\tools\\gcc (
	mkdir ..\\tools\\gcc
)

echo Downloading GCC...
curl -o ..\\tools\\gcc\\gcc.7z -L "https://github.com/brechtsanders/winlibs_mingw/releases/download/%gccVersion%posix-12.0.0-ucrt-r1/winlibs-x86_64-posix-seh-gcc-%gccVersion%-mingw-w64ucrt-12.0.0-r1.7z"
echo Done.
echo.

:: unarchive gcc
echo Extracing GCC...
..\\tools\\7zip-win64\\7za.exe x -y -aoa "..\\tools\\gcc\\gcc.7z" -o"..\\tools\\gcc"
robocopy /nfl /ndl /e "..\\tools\\gcc\\mingw64" "..\\tools\\gcc"
del /q "..\\tools\\gcc\\gcc.7z"
echo Done.
echo.

:: ----------------------------------------------------------------

echo Done!

popd
