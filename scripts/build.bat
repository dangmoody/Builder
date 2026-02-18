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

set binFolder="bin"
set intermediateFolder=%binFolder%"\\intermediate"

if not exist %binFolder% (
	mkdir %binFolder%

	if %errorlevel% NEQ 0 (
		echo ERROR: Failed to create bin folder "%binFolder%"
		exit /B %errorlevel%
	)
)

if not exist %intermediateFolder% (
	mkdir %intermediateFolder%

	if %errorlevel% NEQ 0 (
		echo ERROR: Failed to create intermediate folder "%intermediateFolder%"
		exit /B %errorlevel%
	)
)

set symbols=-g

set optimisation=-O0
if /I [%config%] == [release] (
	set optimisation=-O3
)

set programName=""

set sourceFiles=src\\main.cpp src\\builder.cpp src\\visual_studio.cpp src\\core\\src\\core.suc.cpp src\\backend_clang.cpp src\\backend_msvc.cpp

set defines=-D_CRT_SECURE_NO_WARNINGS -DCORE_USE_XXHASH -DCORE_USE_SUBPROCESS -DCORE_SUC -DHASHMAP_HIDE_MISSING_KEY_WARNING -DHLML_NAMESPACE
if /I [%config%] == [debug] (
	set programName=builder_%config%
	set defines=!defines! -D_DEBUG -DBUILDER_PROGRAM_NAME=\"!programName!\"
) else if /I [%config%] == [release] (
	set programName=builder
	set defines=!defines! -DNDEBUG -DBUILDER_PROGRAM_NAME=\"!programName!\"
)

set includes=-Isrc\\core\\include -Iclang\\include

set libPaths=-Lclang\\lib

set libraries=-luser32.lib -lShlwapi.lib -lDbgHelp.lib -lOle32.lib -lAdvapi32.lib -lOleAut32.lib -llibclang.lib
if /I [%config%] == [debug] (
	set libraries=!libraries! -lmsvcrtd.lib
) else (
	set libraries=!libraries! -lmsvcrt.lib
)

set warningLevels=-Werror -Wall -Wextra -Weverything -Wpedantic

set ignoreWarnings=-Wno-newline-eof -Wno-format-nonliteral -Wno-gnu-zero-variadic-macro-arguments -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -Wno-zero-as-null-pointer-constant -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-missing-field-initializers -Wno-switch-default -Wno-covered-switch-default -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-cast-align -Wno-double-promotion -Wno-nontrivial-memcall -Wno-documentation-unknown-command

set args=clang\\bin\\clang -std=c++20 -o %binFolder%\\%programName%.exe %symbols% %optimisation% %sourceFiles% !defines! %includes% %libPaths% !libraries! %warningLevels% %ignoreWarnings%
echo %args%
%args%

if %errorlevel% NEQ 0 (
	echo ERROR: Build failed
	exit /B %errorlevel%
)

popd
popd

exit /B 0


:ShowUsage
echo Usage: build.bat [debug^|release]
exit /B 1