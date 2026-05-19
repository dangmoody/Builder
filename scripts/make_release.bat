@echo off

set version=%1

if [%version%]==[] (
	echo ERROR: Release version was not set!  Please specify a release version
	goto :ShowUsage
)

pushd %~dp0
pushd ..

:: compile release build
call .\\scripts\\build.bat release

if %errorlevel% NEQ 0 (
	echo ERROR: Failed to create release build! A release cannot be made while the build is broken
	exit /B %errorlevel%
)

:: compile tests
call .\\scripts\\build_tests.bat release

if %errorlevel% NEQ 0 (
	echo ERROR: Failed to build the tests! A release cannot be made while the tests dont compile
	exit /B %errorlevel%
)

:: run tests
pushd tests
call ..\\bin\\builder_tests_release.exe

if %errorlevel% NEQ 0 (
	echo ERROR: Tests failed to run successfully! A release cannot be made while the tests dont work
	exit /B %errorlevel%
)
popd

:: Smoke-test that the trimmed Clang can compile a trivial program
echo Smoke-testing trimmed Clang...
if not exist releases mkdir releases
echo int main(void){} > releases\smoke.c
.\\clang\\bin\\clang.exe -o releases\\smoke_test.exe releases\\smoke.c
if %errorlevel% NEQ 0 (
    echo ERROR: Trimmed Clang failed smoke test -- release cannot be made
    del /q releases\\smoke.c 2>nul
    exit /B 1
)
del /q releases\\smoke.c releases\\smoke_test.exe
echo Done.
echo.

:: now actually make release package
set tempFolder=.\\releases\\temp

robocopy    .\\doc        %tempFolder%\\doc   CHANGELOG.txt
robocopy    .\\doc        %tempFolder%\\doc   Contributing.md
robocopy    .\\bin        %tempFolder%\\bin   builder.exe
robocopy    .\\bin        %tempFolder%\\bin   core.dll
robocopy    .\\clang\\bin %tempFolder%\\bin   libclang.dll
robocopy /e .\\clang      %tempFolder%\\clang

.\\tools\\7zip_win64\\7za.exe a -tzip .\\releases\\builder_%version%_win64.zip %tempFolder%\\bin %tempFolder%\\clang include %tempFolder%\\doc README.md LICENSE

rd /s /Q %tempFolder%

popd
popd

goto :EOF


:ShowUsage
echo Usage:
echo make_release.bat ^<version^>
echo.
echo Arguments:
echo     ^<version^> (required):
echo         The version of the release that you are making (example: v1.2.3 - 1 would be the major version, 2 would be the minor version, and 3 would be the patch version)
goto :EOF
