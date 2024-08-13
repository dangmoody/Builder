@echo off

setlocal EnableDelayedExpansion

set config=%1

pushd %~dp0

set bin_folder=bin\\win64\\%config%

set includes=-I..\\..\\3rdparty\\include

REM build the test DLL so that we can test our dynamic library loading
echo Building test DLL...
..\\..\\the-engine\\tools\\builder\\builder.exe --%config% --out=%bin_folder%\\test_dll.dll test_dll.c %includes%

REM now build tests
echo Building Core tests...
..\\..\\the-engine\\tools\\builder\\builder.exe --%config% --out=%bin_folder%\\core_tests.exe core_tests.cpp %includes%

popd