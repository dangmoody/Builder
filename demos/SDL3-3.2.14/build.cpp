#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
	const std::string sdlBinaryName = "SDL";
	const std::string sdlBinaryFolder = "bin/demos/SDL3";

	BuildConfig sdl = {
		.name			= "sdl",
		.binaryName		= sdlBinaryName,
		.binaryFolder	= sdlBinaryFolder,
		.binaryType		= BINARY_TYPE_DYNAMIC_LIBRARY,
		.sourceFiles = {
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
		.additionalIncludes = {
			"src",	// this feels dirty, are we sure we want to do this?
			"include",
			"include/build_config",
		},
	};

#ifdef _WIN32
	// windows
	// TODO(DM): 14/06/2025: the ONLY reason we cant just do "src/**/windows/*.c" here is because "hidapi/windows/hid.c" includes "hidapi_descriptor_reconstruct.c"
	// so we have to include every windows subfolder manually whilst making sure to exclude only that one file
	// also we apparently only want two source files from "src/thread/generic" so we cant glob that either
	// very annoying!
	sdl.sourceFiles.insert( sdl.sourceFiles.end(), {
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
	sdl.defines.insert( sdl.defines.end(), {
		"SDL_PLATFORM_WIN32",
		"HAVE_MODF"
	} );
	sdl.additionalLibs.insert( sdl.additionalLibs.end(), {
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
#endif

	AddBuildConfig( options, &sdl );

	BuildConfig demo = {
		.name				= "demo",
		.dependsOn			= { sdl },
		.binaryName			= "sdl-demo-app",
		.binaryFolder		= sdlBinaryFolder,
		.sourceFiles		= { "demo-app/*.cpp" },
		.additionalIncludes	= { "include" },
		.additionalLibPaths	= { sdlBinaryFolder },
		.additionalLibs		= { sdlBinaryName },
		.warningsAsErrors	= true,
	};

	AddBuildConfig( options, &demo );
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD
