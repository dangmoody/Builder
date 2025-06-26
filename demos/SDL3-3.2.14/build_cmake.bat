@echo off

setlocal EnableDelayedExpansion

:: Get start time in milliseconds
for /f "tokens=1-4 delims=:.," %%a in ("%TIME%") do (
	set /a "start_ms = (((1%%a%%100)*3600 + %%b*60 + %%c)*1000 + %%d)"
)

set build_folder=build_win64
set visual_studio_folder=build_win64_visual_studio

if exist %build_folder% (
	rmdir /s /q %build_folder%
)

if exist %visual_studio_folder% (
	rmdir /s /q %visual_studio_folder%
)

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

cmake -S . -B %build_folder%
cmake --build %build_folder% --config RelWithDebInfo

:: Get end time in milliseconds
for /f "tokens=1-4 delims=:.," %%a in ("%TIME%") do (
	set /a "end_ms = (((1%%a%%100)*3600 + %%b*60 + %%c)*1000 + %%d)"
)

:: Handle possible midnight rollover
if !end_ms! lss !start_ms! (
	set /a "elapsed_ms = (86400000 - !start_ms!) + !end_ms!"
) else (
	set /a "elapsed_ms = !end_ms! - !start_ms!"
)

echo Total build time: !elapsed_ms! ms