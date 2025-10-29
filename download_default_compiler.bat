@echo off

setlocal EnableDelayedExpansion

set clang_version=20.1.5

:: download versions of clang that we care about
if not exist clang (
	mkdir clang
)

echo Downloading Clang win64 archive...
curl -o clang\\clang_%clang_version%_win64.tar.xz -L "https://github.com/llvm/llvm-project/releases/download/llvmorg-%clang_version%/clang+llvm-%clang_version%-x86_64-pc-windows-msvc.tar.xz"
echo Done.
echo.

:: unarchive curl downloads
echo Unarchiving Clang win64 archive...
.\\7zip\\7za.exe x "clang/clang_%clang_version%_win64.tar.xz" -o"clang"
.\\7zip\\7za.exe x "clang/clang_%clang_version%_win64.tar" -o"clang"
robocopy /nfl /ndl /e "clang\\clang+llvm-%clang_version%-x86_64-pc-windows-msvc" clang
rmdir /s /q "clang\\clang+llvm-%clang_version%-x86_64-pc-windows-msvc"
echo Done.
echo.

:: cleanup - delete the temp files
del /q "clang\\clang_%clang_version%_win64.tar"
del /q "clang\\clang_%clang_version%_win64.tar.xz"

echo Done!