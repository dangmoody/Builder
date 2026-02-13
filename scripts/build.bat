@echo off

setlocal EnableDelayedExpansion

set config=%1

if [%config%]==[] (
	echo ERROR: build config not specified.
	goto :ShowUsage
)

if /I ["%config%"] NEQ ["debug"] (
	if /I ["%config%"] NEQ ["release"] (
		echo ERROR: build config MUST be either "debug", "release", or "retail".
		goto :ShowUsage
	)
)

echo Building config "%config%"...

pushd %~dp0
pushd ..

set bin_folder="bin"
set intermediate_folder=%bin_folder%"\\intermediate"

if not exist %bin_folder% (
	mkdir %bin_folder%

	if %errorlevel% NEQ 0 (
		echo ERROR: Failed to create bin folder "%bin_folder%"
		exit /B %errorlevel%
	)
)

if not exist %intermediate_folder% (
	mkdir %intermediate_folder%

	if %errorlevel% NEQ 0 (
		echo ERROR: Failed to create intermediate folder "%intermediate_folder%"
		exit /B %errorlevel%
	)
)

set symbols=-g

set optimisation=-O0
if /I [%config%] == [release] (
	set optimisation=-O3
)

set program_name=""

set source_files=src\\main.cpp src\\builder.cpp src\\visual_studio.cpp src\\core\\src\\core.suc.cpp src\\backend_clang.cpp src\\backend_msvc.cpp

set defines=-D_CRT_SECURE_NO_WARNINGS -DCORE_USE_XXHASH -DCORE_USE_SUBPROCESS -DCORE_SUC -DHASHMAP_HIDE_MISSING_KEY_WARNING -DHLML_NAMESPACE
if /I [%config%] == [debug] (
	set program_name=builder_%config%
	set defines=!defines! -D_DEBUG -DBUILDER_PROGRAM_NAME=\"!program_name!\"
) else if /I [%config%] == [release] (
	set program_name=builder
	set defines=!defines! -DNDEBUG -DBUILDER_PROGRAM_NAME=\"!program_name!\"
)

set includes=-Isrc\\core\\include

set libraries=-luser32.lib -lShlwapi.lib -lDbgHelp.lib -lOle32.lib -lAdvapi32.lib -lOleAut32.lib
if /I [%config%] == [debug] (
	set libraries=!libraries! -lmsvcrtd.lib
) else (
	set libraries=!libraries! -lmsvcrt.lib
)

set warning_levels=-Werror -Wall -Wextra -Weverything -Wpedantic

set ignore_warnings=-Wno-newline-eof -Wno-format-nonliteral -Wno-gnu-zero-variadic-macro-arguments -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -Wno-zero-as-null-pointer-constant -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-missing-field-initializers -Wno-switch-default -Wno-covered-switch-default -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-cast-align -Wno-double-promotion -Wno-nontrivial-memcall

set args=clang\\bin\\clang -std=c++20 -o %bin_folder%\\%program_name%.exe %symbols% %optimisation% %source_files% !defines! %includes% !libraries! %warning_levels% %ignore_warnings%
echo %args%
%args%

if %errorlevel% NEQ 0 (
	echo ERROR: Build failed
	exit /B %errorlevel%
)

::copy %bin_folder%\\builder_%config%.exe %bin_folder%\\builder.exe

popd
popd

exit /B 0


:ShowUsage
echo Usage: build.bat [debug^|release]
exit /B 1