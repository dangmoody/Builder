:: TODO: DM: 19/08/2026: if we can have "empty" BuildConfigs that dont actually do any building, does that mean we can have a build.c create a release for us too?

@echo off

set version=%1

if [%version%]==[] (
	echo ERROR: Release version was not set!  Please specify a release version
	goto :ShowUsage
)

pushd %~dp0
pushd ..

:: build and run tests
pushd tests

.\\build_tests.exe

if %errorlevel% NEQ 0 (
	echo ERROR: Failed to build the tests!  A release cannot be made while the tests dont compile
	exit /B %errorlevel%
)

.\\builder_tests.exe

if %errorlevel% NEQ 0 (
	echo ERROR: Tests failed to run successfully!  A release cannot be made while the tests dont work
	exit /B %errorlevel%
)

popd

if exist .\\releases\\builder_%version%.zip del .\\releases\\builder_%version%.zip

.\\tools\\7zip-win64\\7za.exe a -tzip .\\releases\\builder_%version%.zip builder.h builder_visual_studio.h builder_vs_code.h builder_zed.h builder_compilation_database.h doc\\CHANGELOG.txt doc\\CHANGELOG_OLD.txt doc\\Contributing.md README.md LICENSE

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