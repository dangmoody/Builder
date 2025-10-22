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

echo Downloading Clang win64 archive...
curl -o clang_win64\\clang_%clang_version%_win64.tar.xz -L "https://github.com/llvm/llvm-project/releases/download/llvmorg-%clang_version%/clang+llvm-%clang_version%-x86_64-pc-windows-msvc.tar.xz"
echo Done.
echo.

REM unarchive curl downloads
echo Unarchiving Clang win64 archive...
.\\7zip\\7za.exe x "clang_win64/clang_%clang_version%_win64.tar.xz" -o"clang_win64"
.\\7zip\\7za.exe x "clang_win64/clang_%clang_version%_win64.tar" -o"clang_win64"
robocopy /nfl /ndl /e "clang_win64\\clang+llvm-%clang_version%-x86_64-pc-windows-msvc" clang_win64
rmdir /s /q "clang_win64\\clang+llvm-%clang_version%-x86_64-pc-windows-msvc"
echo Done.
echo.

REM cleanup - delete the temp files
del /q "clang_win64\\clang_%clang_version%_win64.tar"
del /q "clang_win64\\clang_%clang_version%_win64.tar.xz"

echo Done!