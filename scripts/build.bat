@echo off

setlocal EnableDelayedExpansion

call "%~dp0build_common.bat" %1
if %errorlevel% NEQ 0 exit /B %errorlevel%

echo Building config "%config%"...

set ignoreWarnings=!ignoreWarnings! -Wno-cast-align -Wno-nontrivial-memcall

set sourceFiles=src\\main.cpp src\\builder.cpp src\\visual_studio.cpp src\\core\\src\\core.suc.cpp src\\backend_clang.cpp src\\backend_msvc.cpp src\\win_support.cpp src\\vs_code.cpp src\\zed_editor.cpp

set args=clang\\bin\\clang -Xlinker /NODEFAULTLIB -std=c++20 -o %binFolder%\\%programName%.exe %symbols% %optimisation% %sourceFiles% !defines! %includes% %libPaths% !libraries! %warningLevels% !ignoreWarnings!
echo !args!
!args!

if %errorlevel% NEQ 0 (
	echo ERROR: Build failed
	exit /B %errorlevel%
)

REM We have to build builder before running tests so don't share functionality
echo Copying libclang.dll to %binFolder%...
copy /y clang\\bin\\libclang.dll %binFolder%\\libclang.dll

popd
popd

exit /B 0
