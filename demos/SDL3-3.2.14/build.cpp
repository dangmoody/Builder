#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	const std::string sdl_binary_name = "SDL";
	const std::string sdl_binary_folder = "bin/demos/SDL3";

	BuildConfig config_sdl_common = {
		.binary_name	= sdl_binary_name,
		.binary_folder	= sdl_binary_folder,
		.binary_type	= BINARY_TYPE_DYNAMIC_LIBRARY,
		.source_files = {
			"src/*.c",
			"src/atomic/*.c",
			"src/audio/*.c",
			"src/audio/disk/*.c",
			"src/audio/dummy/*.c",
			"src/camera/*.c",
			"src/camera/dummy/*.c",
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
			"src/render/software/*.c",
			"src/sensor/*.c",
			"src/stdlib/*.c",
			"src/storage/*.c",
			"src/storage/generic/*.c",
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
			"DLL_EXPORT",
		},
		.additional_includes = {
			"src",	// this feels dirty, are we sure we want to do this?
			"include",
			"include/build_config",
		},
	};

	// windows
	// TODO(DM): 14/06/2025: the ONLY reason we cant just do "src/**/windows/*.c" here is because "hidapi/windows/hid.c" includes "hidapi_descriptor_reconstruct.c"
	// so we have to include every windows subfolder manually whilst making sure to exclude only that one file
	// very annoying!
	BuildConfig config_sdl_windows = config_sdl_common;
	config_sdl_windows.name = "sdl-windows";
	config_sdl_windows.source_files.insert( config_sdl_windows.source_files.end(), {
		"src/audio/directsound/*.c",
		"src/audio/wasapi/*.c",
		"src/core/windows/*.c",
		"src/dialog/windows/*.c",
		"src/filesystem/windows/*.c",
		"src/haptic/windows/*.c",
		"src/hidapi/windows/hid.c",
		"src/io/windows/*.c",
		"src/joystick/gdk/*.c",
		"src/joystick/windows/*.c",
		"src/gpu/vulkan/*.c",
		"src/gpu/d3d12/*.c",
		"src/loadso/windows/*.c",
		"src/locale/windows/*.c",
		"src/main/windows/*.c",
		"src/misc/windows/*.c",
		"src/power/windows/*.c",
		"src/process/windows/*.c",
		"src/render/direct3d/*.c",
		"src/render/direct3d11/*.c",
		"src/render/direct3d12/*.c",
		"src/render/gpu/*.c",
		"src/render/opengl/*.c",
		"src/render/opengles2/*.c",
		"src/render/vulkan/*.c",
		"src/sensor/windows/*.c",
		"src/time/windows/*.c",
		"src/timer/windows/*.c",
		"src/thread/generic/SDL_syscond.c",
		"src/thread/generic/SDL_sysrwlock.c",
		"src/thread/windows/*.c",
		"src/tray/windows/*.c",
		"src/video/windows/*.c"
	} );
	config_sdl_windows.defines.insert( config_sdl_windows.defines.end(), {
		"SDL_PLATFORM_WIN32",
		"HAVE_MODF"
	} );
	config_sdl_windows.additional_libs.insert( config_sdl_windows.additional_libs.end(), {
		"Ole32.lib",
		"OleAut32.lib",
		"Winmm.lib",
		"Imm32.lib",
		"Advapi32.lib",
		"Shell32.lib",
		"Cfgmgr32.lib",
		"Gdi32.lib",
		"SetupAPI.lib",
		"Version.lib"
	} );
	add_build_config( options, &config_sdl_windows );

	BuildConfig config_demo_windows = {
		.name					= "sdl-demo-windows",
		.depends_on				= { config_sdl_windows },
		.binary_name			= "sdl-demo-app",
		.binary_folder			= sdl_binary_folder,
		.source_files			= { "demo-app/*.cpp" },
		.additional_includes	= { "include" },
		.additional_lib_paths	= { sdl_binary_folder },
		.additional_libs		= { sdl_binary_name },
		.warnings_as_errors		= true,
	};

	add_build_config( options, &config_demo_windows );
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD
