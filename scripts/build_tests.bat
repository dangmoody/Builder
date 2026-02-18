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
pushd ..

set binFolder="bin"
set intermediatePath=%binFolder%"\\intermediate"

if not exist %binFolder% (
	mkdir %binFolder%
)

if not exist %intermediatePath% (
	mkdir %intermediatePath%
)

set symbols="-g"

set optimisation=""
if /I [%config%]==[release] (
	set optimisation=-O3 -ffast-math
)

set programName=""

set sourceFiles=tests\\tests_main.cpp src\\builder.cpp src\\visual_studio.cpp src\\backend_clang.cpp src\\backend_msvc.cpp src\\core\\src\\core.suc.cpp

set defines=-D_CRT_SECURE_NO_WARNINGS -DCORE_USE_XXHASH -DCORE_USE_SUBPROCESS -DCORE_SUC -DHASHMAP_HIDE_MISSING_KEY_WARNING -DHLML_NAMESPACE
if /I [%config%]==[debug] (
	set programName=builder_%config%
	set defines=!defines! -D_DEBUG -DBUILDER_PROGRAM_NAME=\"!programName!\"
) else if /I [%config%]==[release] (
	set programName=builder
	set defines=!defines! -DNDEBUG -DBUILDER_PROGRAM_NAME=\"!programName!\"
)

set includes=-Isrc/core/include -Iclang\\include

set libPaths=-Lclang\\lib

set libraries=-luser32.lib -lShlwapi.lib -lDbgHelp.lib -lOle32.lib -lAdvapi32.lib -lOleAut32.lib -llibclang.lib
if /I [%config%]==[debug] (
	set libraries=!libraries! -lmsvcrtd.lib
)

set warningLevels=-Werror -Wall -Wextra -Weverything -Wpedantic

set ignoreWarnings=-Wno-newline-eof -Wno-format-nonliteral -Wno-gnu-zero-variadic-macro-arguments -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -Wno-zero-as-null-pointer-constant -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-missing-field-initializers -Wno-switch-default -Wno-covered-switch-default -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-double-promotion -Wno-documentation-unknown-command

set args=clang\\bin\\clang -std=c++20 -o %binFolder%\\builder_tests_%config%.exe %symbols% %optimisation% %sourceFiles% !defines! %includes% %libPaths% !libraries! %warningLevels% %ignoreWarnings%
echo %args%
%args%

copy clang\\bin\\libclang.dll bin\\libclang.dll

popd
popd

goto :EOF


:ShowUsage
echo Usage: build_tests.bat [debug^|release]

goto :EOF