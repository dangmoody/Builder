#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	const std::string sdlBinaryName = "SDL";
	const std::string sdlBinaryFolder = "bin/demos/SDL3";

	BuildConfig configSDLCommon = {
		.binary_name	= sdlBinaryName,
		.binary_folder	= sdlBinaryFolder,
		.binary_type	= BINARY_TYPE_DYNAMIC_LIBRARY,
		.source_files = {
			"src/*.c",
			"src/atomic/*.c",
			"src/audio/*.c",
			"src/camera/*.c",
			"src/camera/mediafoundation/*.c",
			"src/core/*.c",
			"src/cpuinfo/*.c",
			"src/dialog/*.c",
			"src/dynapi/*.c",
			"src/events/*.c",
			"src/filesystem/*.c",
			"src/gpu/*.c",
			"src/haptic/*.c",
			"src/hidapi/*.c",
			"src/io/*.c",
			"src/io/generic/*.c",
			"src/joystick/*.c",
			"src/joystick/hidapi/*.c",
			"src/joystick/virtual/*.c",
			"src/locale/*.c",
			"src/main/*.c",
			"src/main/generic/*.c",
			"src/misc/*.c",
			"src/power/*.c",
			"src/process/*.c",
			"src/render/*.c",
			"src/sensor/*.c",
			"src/stdlib/*.c",
			"src/storage/*.c",
			"src/tray/*.c",
			"src/thread/*.c",
			"src/time/*.c",
			"src/timer/*.c",
			"src/video/*.c",
			"src/video/dummy/*.c",
			"src/video/offscreen/*.c",
			"src/video/yuv2rgb/*.c",
		},
		.defines = {
			"SDL_PLATFORM_WIN32",
		},
		.additional_includes = {
			"src",	// this feels dirty, are we sure we want to do this?
			"include",
			"include/build_config",
		},
	};

	// windows
	BuildConfig configSDLWin64 = configSDLCommon;
	configSDLWin64.name = "windows";
	configSDLWin64.source_files.push_back( "src/audio/dummy/*.c" );
	configSDLWin64.source_files.push_back( "src/audio/disk/*.c" );
	configSDLWin64.source_files.push_back( "src/audio/directsound/*.c" );
	configSDLWin64.source_files.push_back( "src/audio/wasapi/*.c" );
	configSDLWin64.source_files.push_back( "src/camera/dummy/*.c" );
	configSDLWin64.source_files.push_back( "src/core/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/dialog/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/filesystem/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/gpu/vulkan/*.c" );
	configSDLWin64.source_files.push_back( "src/gpu/d3d12/*.c" );
	configSDLWin64.source_files.push_back( "src/haptic/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/io/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/joystick/gdk/*.c" );
	configSDLWin64.source_files.push_back( "src/joystick/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/loadso/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/locale/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/main/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/misc/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/power/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/process/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/render/direct3d/*.c" );
	configSDLWin64.source_files.push_back( "src/render/direct3d11/*.c" );
	configSDLWin64.source_files.push_back( "src/render/direct3d12/*.c" );
	configSDLWin64.source_files.push_back( "src/render/gpu/*.c" );
	configSDLWin64.source_files.push_back( "src/render/opengl/*.c" );
	configSDLWin64.source_files.push_back( "src/render/opengles2/*.c" );
	configSDLWin64.source_files.push_back( "src/render/software/*.c" );
	configSDLWin64.source_files.push_back( "src/render/vulkan/*.c" );
	configSDLWin64.source_files.push_back( "src/sensor/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/storage/generic/*.c" );
	configSDLWin64.source_files.push_back( "src/storage/steam/*.c" );
	configSDLWin64.source_files.push_back( "src/thread/generic/SDL_syscond.c" );
	configSDLWin64.source_files.push_back( "src/thread/generic/SDL_sysrwlock.c" );
	configSDLWin64.source_files.push_back( "src/thread/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/time/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/timer/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/tray/windows/*.c" );
	configSDLWin64.source_files.push_back( "src/video/windows/*.c" );
	// configSDLWin64.source_files.push_back( "src/**/directsound/*.c" );
	// configSDLWin64.source_files.push_back( "src/**/gdk/*.c" );
	// configSDLWin64.source_files.push_back( "src/**/wasapi/*.c" );
	// configSDLWin64.source_files.push_back( "src/**/windows/*.c" );
	configSDLWin64.defines.push_back( "HAVE_MODF" );
	configSDLWin64.additional_libs.push_back( "Ole32.lib" );
	configSDLWin64.additional_libs.push_back( "OleAut32.lib" );
	configSDLWin64.additional_libs.push_back( "Winmm.lib" );
	configSDLWin64.additional_libs.push_back( "Imm32.lib" );
	configSDLWin64.additional_libs.push_back( "Advapi32.lib" );
	configSDLWin64.additional_libs.push_back( "Shell32.lib" );
	configSDLWin64.additional_libs.push_back( "Cfgmgr32.lib" );
	configSDLWin64.additional_libs.push_back( "Gdi32.lib" );
	configSDLWin64.additional_libs.push_back( "SetupAPI.lib" );
	configSDLWin64.additional_libs.push_back( "Version.lib" );
	add_build_config( options, &configSDLWin64 );

	BuildConfig configDemoWindows = {
		.name					= "sdl-demo-windows",
		.depends_on				= { configSDLWin64 },
		.binary_name			= sdlBinaryFolder,
		.source_files			= { "demo-app/*.cpp" },
		.additional_includes	= { "include" },
		.additional_lib_paths	= { sdlBinaryFolder },
		.additional_libs		= { sdlBinaryName },
		.warnings_as_errors		= true,
	};

	add_build_config( options, &configDemoWindows );
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD