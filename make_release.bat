@echo off

set version=%1

if [%version%]==[] (
	echo ERROR: Release version was not set!  Please specify a release version
	goto :ShowUsage
)

call build.bat release

pushd %~dp0

set temp_folder=.\\releases\\builder

if not exist %temp_folder% (
	mkdir %temp_folder%
)

robocopy .\\    %temp_folder% builder.exe
robocopy .\\    %temp_folder% builder.h
robocopy .\\doc %temp_folder% CHANGELOG.txt

.\\7zip\\7za.exe a -tzip releases\\builder_%version%.zip %temp_folder%

rd /s /Q %temp_folder%

popd

goto :EOF


:ShowUsage
echo Usage:
echo make_release.bat ^<version^>
echo.
echo Arguments:
echo     ^<version^> (required):
echo         The version of the release that you are making (for example: 1.2.3 - in this example: 1 would be the major version, 2 would be the minor version, and 3 would be the patch version)
goto :EOF
