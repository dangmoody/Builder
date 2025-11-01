@echo off

set version=%1

if [%version%]==[] (
	echo ERROR: Release version was not set!  Please specify a release version
	goto :ShowUsage
)

pushd %~dp0
pushd ..

call .\\scripts\\build.bat release

set temp_folder=.\\releases\\temp

robocopy    .\\bin\\win64\\release %temp_folder%\\bin   builder.exe
robocopy /e .\\clang               %temp_folder%\\clang

.\\tools\\7zip_win64\\7za.exe a -tzip .\\releases\\builder_%version%_win64.zip %temp_folder%\\bin %temp_folder%\\clang include doc README.md LICENSE

rd /s /Q %temp_folder%

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
