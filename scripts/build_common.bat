@echo off

REM Ensure usage is correct
set config=%1
if [%config%]==[] (
	echo ERROR: build config not specified.
	goto :ShowUsage
)

if /I ["%config%"] NEQ ["debug"] (
	if /I ["%config%"] NEQ ["release"] (
		echo ERROR: build config MUST be either "debug" or "release".
		goto :ShowUsage
	)
)

REM Scope to the folder one level up from the script's folder
pushd %~dp0
pushd ..

REM Create bin and intermediate folders if they don't exist
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

REM Set up config specific environment variables
if /I [%config%] == [debug] (
	set symbols=-g
	set optimisation=-O0
	set programName=builder_debug
	set defines=-D_CRT_SECURE_NO_WARNINGS -DCORE_USE_XXHASH -DCORE_USE_SUBPROCESS -DCORE_SUC -DHASHMAP_HIDE_MISSING_KEY_WARNING^
 -DHLML_NAMESPACE -D_DEBUG -DBUILDER_PROGRAM_NAME=\"builder_debug\"

	set libraries=-luser32.lib -lShlwapi.lib -lDbgHelp.lib -lOle32.lib -lAdvapi32.lib -lOleAut32.lib -llibclang.lib -lkernel32.lib^
 -lmsvcrtd.lib -lmsvcprtd.lib -lvcruntimed.lib -lucrtd.lib
) 

if /I [%config%] == [release]  (
	set symbols=
	set optimisation=-O3
	set programName=builder
	set defines=-D_CRT_SECURE_NO_WARNINGS -DCORE_USE_XXHASH -DCORE_USE_SUBPROCESS -DCORE_SUC -DHASHMAP_HIDE_MISSING_KEY_WARNING^
 -DHLML_NAMESPACE -DNDEBUG -DBUILDER_PROGRAM_NAME=\"builder\"

	set libraries=-luser32.lib -lShlwapi.lib -lDbgHelp.lib -lOle32.lib -lAdvapi32.lib -lOleAut32.lib -llibclang.lib -lkernel32.lib^
 -lmsvcrt.lib -lmsvcprt.lib -lvcruntime.lib -lucrt.lib
)

REM Set up shared environment variables
set includes=-Isrc\\core\\include -Iclang\\include

set libPaths=-Lclang\\lib

set warningLevels=-Werror -Wall -Wextra -Weverything -Wpedantic

set ignoreWarnings=-Wno-newline-eof -Wno-format-nonliteral -Wno-gnu-zero-variadic-macro-arguments -Wno-declaration-after-statement^
 -Wno-unsafe-buffer-usage -Wno-zero-as-null-pointer-constant -Wno-c++98-compat-pedantic -Wno-old-style-cast^
 -Wno-missing-field-initializers -Wno-switch-default -Wno-covered-switch-default -Wno-unused-function -Wno-unused-variable^
 -Wno-unused-but-set-variable -Wno-double-promotion -Wno-documentation-unknown-command -Wno-switch

exit /B 0

REM Show usage and exit with error
:ShowUsage
echo Usage: build.bat [debug^|release]
exit /B 1