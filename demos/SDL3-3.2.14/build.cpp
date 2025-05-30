#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig common = {
		.binary_name	= "SDL",
		.binary_folder	= "bin/demos/SDL3",
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
	BuildConfig configWindows = common;
	configWindows.name = "windows";
	configWindows.source_files.push_back( "src/audio/dummy/*.c" );
	configWindows.source_files.push_back( "src/audio/disk/*.c" );
	configWindows.source_files.push_back( "src/audio/directsound/*.c" );
	configWindows.source_files.push_back( "src/audio/wasapi/*.c" );
	configWindows.source_files.push_back( "src/camera/dummy/*.c" );
	configWindows.source_files.push_back( "src/core/windows/*.c" );
	configWindows.source_files.push_back( "src/dialog/windows/*.c" );
	configWindows.source_files.push_back( "src/filesystem/windows/*.c" );
	configWindows.source_files.push_back( "src/gpu/vulkan/*.c" );
	configWindows.source_files.push_back( "src/gpu/d3d12/*.c" );
	configWindows.source_files.push_back( "src/haptic/windows/*.c" );
	configWindows.source_files.push_back( "src/io/windows/*.c" );
	configWindows.source_files.push_back( "src/joystick/gdk/*.c" );
	configWindows.source_files.push_back( "src/joystick/windows/*.c" );
	configWindows.source_files.push_back( "src/loadso/windows/*.c" );
	configWindows.source_files.push_back( "src/locale/windows/*.c" );
	configWindows.source_files.push_back( "src/main/windows/*.c" );
	configWindows.source_files.push_back( "src/misc/windows/*.c" );
	configWindows.source_files.push_back( "src/power/windows/*.c" );
	configWindows.source_files.push_back( "src/process/windows/*.c" );
	configWindows.source_files.push_back( "src/render/direct3d/*.c" );
	configWindows.source_files.push_back( "src/render/direct3d11/*.c" );
	configWindows.source_files.push_back( "src/render/direct3d12/*.c" );
	configWindows.source_files.push_back( "src/render/gpu/*.c" );
	configWindows.source_files.push_back( "src/render/opengl/*.c" );
	configWindows.source_files.push_back( "src/render/opengles2/*.c" );
	configWindows.source_files.push_back( "src/render/software/*.c" );
	configWindows.source_files.push_back( "src/render/vulkan/*.c" );
	configWindows.source_files.push_back( "src/sensor/windows/*.c" );
	configWindows.source_files.push_back( "src/storage/generic/*.c" );
	configWindows.source_files.push_back( "src/storage/steam/*.c" );
	configWindows.source_files.push_back( "src/thread/generic/SDL_syscond.c" );
	configWindows.source_files.push_back( "src/thread/generic/SDL_sysrwlock.c" );
	configWindows.source_files.push_back( "src/thread/windows/*.c" );
	configWindows.source_files.push_back( "src/time/windows/*.c" );
	configWindows.source_files.push_back( "src/timer/windows/*.c" );
	configWindows.source_files.push_back( "src/tray/windows/*.c" );
	configWindows.source_files.push_back( "src/video/windows/*.c" );
	// configWindows.source_files.push_back( "src/**/directsound/*.c" );
	// configWindows.source_files.push_back( "src/**/gdk/*.c" );
	// configWindows.source_files.push_back( "src/**/wasapi/*.c" );
	// configWindows.source_files.push_back( "src/**/windows/*.c" );
	configWindows.defines.push_back( "HAVE_MODF" );
	configWindows.additional_libs.push_back( "Ole32.lib" );
	configWindows.additional_libs.push_back( "OleAut32.lib" );
	configWindows.additional_libs.push_back( "Winmm.lib" );
	configWindows.additional_libs.push_back( "Imm32.lib" );
	configWindows.additional_libs.push_back( "Advapi32.lib" );
	configWindows.additional_libs.push_back( "Shell32.lib" );
	configWindows.additional_libs.push_back( "Cfgmgr32.lib" );
	configWindows.additional_libs.push_back( "Gdi32.lib" );
	configWindows.additional_libs.push_back( "SetupAPI.lib" );
	configWindows.additional_libs.push_back( "Version.lib" );
	add_build_config( options, &configWindows );
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD