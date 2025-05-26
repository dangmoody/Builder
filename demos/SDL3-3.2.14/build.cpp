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
			"src/gpu/*.c",
			"src/io/*.c",
			"src/io/generic/*.c",
			"src/filesystem/*.c",
			"src/gpu/*.c",
			"src/joystick/*.c",
			"src/joystick/virtual/*.c",
			"src/haptic/*.c",
			"src/hidapi/*.c",
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
			// "src/video/dummy/*.c",
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
		.additional_libs = {
			"ole32.lib"
		},
	};

	// windows
	BuildConfig configWindows = common;
	configWindows.name = "windows";
	configWindows.source_files.push_back( "src/audio/dummy/*.c" );
	configWindows.source_files.push_back( "src/audio/disk/*.c" );
	configWindows.source_files.push_back( "src/camera/dummy/*.c" );
	configWindows.source_files.push_back( "src/storage/generic/*.c" );
	configWindows.source_files.push_back( "src/storage/steam/*.c" );
	configWindows.source_files.push_back( "src/thread/generic/SDL_syscond.c" );
	configWindows.source_files.push_back( "src/thread/generic/SDL_sysrwlock.c" );
	configWindows.source_files.push_back( "src/**/d3d12/*.c" );
	configWindows.source_files.push_back( "src/**/directsound/*.c" );
	configWindows.source_files.push_back( "src/**/gdk/*.c" );
	configWindows.source_files.push_back( "src/**/vulkan/*.c" );
	configWindows.source_files.push_back( "src/**/wasapi/*.c" );
	configWindows.source_files.push_back( "src/**/windows/*.c" );
	add_build_config( options, &configWindows );
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD