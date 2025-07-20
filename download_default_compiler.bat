@echo off

setlocal EnableDelayedExpansion

REM echo WARNING: RUN THIS SCRIPT AS ADMINISTRATOR
REM echo OTHERWISE IT WONT WORK PROPERLY

REM get clang version and check its not empty
set clang_version=%1
if /I ["%clang_version%"]==[""] (
	echo "No clang version specified."
	goto :EOF
)

REM download versions of clang that we care about
if not exist clang_win64 (
	mkdir clang_win64
)

if not exist clang_linux (
	mkdir clang_linux
)

echo Downloading Clang win64 archive...
curl -o %clang_win64%\\clang_%clang_version%_win64.tar.xz -L "https://github.com/llvm/llvm-project/releases/download/llvmorg-%clang_version%/clang+llvm-%clang_version%-x86_64-pc-windows-msvc.tar.xz"
echo Done.
echo.

echo Downloading Clang linux archive...
curl -o %clang_linux%\\clang_%clang_version%_linux.tar.xz -L "https://github.com/llvm/llvm-project/releases/download/llvmorg-%clang_version%/LLVM-%clang_version%-Linux-X64.tar.xz"
echo Done.
echo.

REM REM unarchive curl downloads
REM echo Unarchiving Clang win64 archive...
REM .\\7zip\\7za.exe x "%temp_folder%/clang_%clang_version%_win64.tar.xz" -o"clang_win64"
REM .\\7zip\\7za.exe x "%temp_folder%/clang_%clang_version%_win64.tar" -o"clang_win64"
REM robocopy /nfl /ndl /e "clang_win64\\clang+llvm-%clang_version%-x86_64-pc-windows-msvc" clang_win64
REM rmdir /s /q "clang_win64\\clang+llvm-%clang_version%-x86_64-pc-windows-msvc"
REM echo Done.
REM echo.

REM echo Unarchiving Clang linux archive...
REM .\\7zip\\7za.exe x "%temp_folder%/clang_%clang_version%_linux.tar.xz" -o"clang_linux"
REM .\\7zip\\7za.exe x "%temp_folder%/clang_%clang_version%_linux.tar" -o"clang_linux"
REM robocopy /nfl /ndl /e "clang_linux\\LLVM-20.1.5-Linux-X64" clang_linux
REM rmdir /s /q "clang_linux\\LLVM-20.1.5-Linux-X64"
REM echo Done.
REM echo.

REM cleanup - delete the temp files
REM del /q "%clang_win64%\\clang_%clang_version%_win64.tar.xz"
REM del /q "%clang_linux%\\clang_%clang_version%_linux.tar.xz"

echo Done!