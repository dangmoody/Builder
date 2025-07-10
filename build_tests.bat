@echo off

setlocal EnableDelayedExpansion

set config=%1

if [%config%]==[] (
	echo ERROR: build config not specified.
	goto :ShowUsage
)

if ["%config%"] NEQ ["debug"] (
	if ["%config%"] NEQ ["release"] (
		echo ERROR: build config MUST be either "debug" or "release".
		goto :ShowUsage
	)
)

pushd %~dp0

set bin_folder="bin\\win64\\"%config%
set intermediate_path=%bin_folder%"\\intermediate"

if not exist %bin_folder% (
	mkdir %bin_folder%
)

if not exist %intermediate_path% (
	mkdir %intermediate_path%
)

set symbols=""
if /I [%config%]==[debug] (
	set symbols=-g
)

set optimisation=""
if /I [%config%]==[release] (
	set optimisation=-O3 -ffast-math
)

set source_files=tests\\tests_main.cpp

set defines=-D_CRT_SECURE_NO_WARNINGS -DCORE_SUC -DCORE_USE_SUBPROCESS
if /I [%config%]==[debug] (
	set defines=!defines! -D_DEBUG
)

if /I [%config%]==[release] (
	set defines=!defines! -DNDEBUG
)

set includes=-Isrc/core/include

set libraries=-luser32.lib -lShlwapi.lib -lDbgHelp.lib
if /I [%config%]==[debug] (
	set libraries=!libraries! -lmsvcrtd.lib
)

set warning_levels=-Werror -Wall -Wextra -Weverything -Wpedantic

set ignore_warnings=-Wno-newline-eof -Wno-format-nonliteral -Wno-gnu-zero-variadic-macro-arguments -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -Wno-zero-as-null-pointer-constant -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-missing-field-initializers -Wno-switch-default -Wno-covered-switch-default -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-double-promotion

clang -std=c++20 -o %bin_folder%\\builder_tests.exe %symbols% %optimisation% %source_files% !defines! %includes% !libraries! %warning_levels% %ignore_warnings%

xcopy /v /y /f %bin_folder%\\builder_tests.exe .\\

popd

goto :EOF


:ShowUsage
echo Usage: build.bat [debug^|release]

goto :EOF