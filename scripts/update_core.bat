@echo off

pushd %~dp0

builder ..\\..\\core\\build.cpp --config=core --release --force-rebuild

echo Copying core include folder...
robocopy /e ..\\..\\core\\include      ..\\src\\core\\include

echo Copying core src folder...
robocopy /e ..\\..\\core\\src          ..\\src\\core\\src

echo Copying core.dll...
robocopy    ..\\..\\core\\bin\\debug ..\\3rdparty\\core core.dll

echo Copying core.lib...
robocopy    ..\\..\\core\\bin\\debug ..\\3rdparty\\core core.lib

echo Copying core.pdb...
robocopy    ..\\..\\core\\bin\\debug ..\\3rdparty\\core core.pdb

echo Done.
echo.

popd
