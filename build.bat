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

echo Building config "%config%"...

pushd %~dp0

set bin_folder="bin\\win64\\"%config%
set intermediate_path=%bin_folder%"\\intermediate"

if not exist %bin_folder% (
	mkdir %bin_folder%
)

if not exist %intermediate_path% (
	mkdir %intermediate_path%
)

set symbols=-g
REM if /I [%config%]==[debug] (
REM 	set symbols=-g
REM )

set optimisation=""
if /I [%config%]==[release] (
	set optimisation=-O3
)

set source_files=src\\builder.cpp src\\visual_studio.cpp src\\core\\src\\core.suc.cpp

set defines=-D_CRT_SECURE_NO_WARNINGS -DCORE_USE_XXHASH -DCORE_SUC -DHASHMAP_HIDE_MISSING_KEY_WARNING
if /I [%config%]==[debug] (
	set defines=!defines! -D_DEBUG
)

if /I [%config%]==[release] (
	set defines=!defines! -DNDEBUG
)

set includes=-Isrc\\core\\include

set libraries=-luser32.lib -lShlwapi.lib -lDbgHelp.lib -lOle32.lib
if /I [%config%]==[debug] (
	set libraries=!libraries! -lmsvcrtd.lib
) else (
	set libraries=!libraries! -lmsvcrt.lib
)

set warning_levels=-Werror -Wall -Wextra -Weverything -Wpedantic

set ignore_warnings=-Wno-newline-eof -Wno-format-nonliteral -Wno-gnu-zero-variadic-macro-arguments -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -Wno-zero-as-null-pointer-constant -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-missing-field-initializers -Wno-switch-default -Wno-covered-switch-default -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-cast-align -Wno-double-promotion

set args=clang -std=c++20 -o %bin_folder%\\builder.exe %symbols% %optimisation% %source_files% !defines! %includes% !libraries! %warning_levels% %ignore_warnings%
echo %args%
%args%

xcopy /v /y /f %bin_folder%\\builder.exe .\\

popd

goto :EOF


:ShowUsage
echo Usage: build.bat [debug^|release]

goto :EOF