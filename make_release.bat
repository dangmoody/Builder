@echo off

set version=%1

if [%version%]==[] (
	echo ERROR: Release version was not set!  Please specify a release version
	goto :ShowUsage
)

call build.bat release

pushd %~dp0

set temp_folder=.\\releases\\temp

if not exist %temp_folder% (
	mkdir %temp_folder%
)

if not exist %temp_folder%\\clang (
	mkdir %temp_folder%\\clang
)

robocopy    .\\            %temp_folder%        builder.exe
robocopy    .\\            %temp_folder%        builder.h
robocopy    .\\            %temp_folder%        README.md
robocopy    .\\            %temp_folder%        LICENSE
robocopy    .\\doc         %temp_folder%        CHANGELOG.txt
robocopy    .\\doc         %temp_folder%        Contributing.md
robocopy /e .\\clang_win64 %temp_folder%\\clang

.\\7zip\\7za.exe a -tzip releases\\builder_win64_%version%.zip %temp_folder%

rd /s /Q %temp_folder%

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
