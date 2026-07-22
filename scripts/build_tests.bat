@echo off

setlocal EnableDelayedExpansion

call "%~dp0build_common.bat" %1
if %errorlevel% NEQ 0 exit /B %errorlevel%

echo Building tests config "%config%"...

if /I [%config%] == [release] set optimisation=!optimisation! -ffast-math

set sourceFiles=tests\\tests_main.cpp src\\builder.cpp src\\visual_studio.cpp src\\backend_clang.cpp src\\backend_msvc.cpp src\\win_support.cpp src\\vs_code.cpp src\\zed_editor.cpp^
	src\\debug.cpp src\\file.cpp src\\hash.cpp src\\hashmap.cpp src\\linear_allocator.cpp src\\math.cpp src\\paths.cpp src\\stb_impl.cpp src\\string.cpp src\\string_builder.cpp src\\temp_storage.cpp^
	src\\win64\\*.cpp

set args=clang\\bin\\clang -Xlinker /NODEFAULTLIB -std=c++20 -o %binFolder%\\builder_tests_%config%.exe %symbols% %optimisation% %sourceFiles% !defines! %includes% %libPaths% !libraries! %warningLevels% %ignoreWarnings%
echo !args!
!args!

if %errorlevel% NEQ 0 (
	echo ERROR: Build failed
	exit /B %errorlevel%
)

popd
popd

exit /B 0