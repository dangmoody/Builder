#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

int main( int argc, char **argv ) {
	BuilderOptions options = {
		.argc = argc,
		.argv = argv,
	};

	Builder_RebuildSelf(argc,argv);

	const char *sdlBinaryName = "SDL";
	const char *sdlBinaryFolder = "bin/demos/SDL3";

	BuildConfig sdl = {
		.name			= "sdl",
		.binaryName		= sdlBinaryName,
		.binaryFolder	= sdlBinaryFolder,
		.binaryType		= BINARY_TYPE_DYNAMIC_LIBRARY,
		.sourceFiles = (const char *[]) {
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
			"src/haptic/hidapi/*.c",
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
#if defined( _WIN32 )
			// TODO(DM): 14/06/2025: we cant just do "src/**/windows/*.c" here because
			//	- "hidapi/windows/hid.c" includes "hidapi_descriptor_reconstruct.c" which we dont want to use on windows and it isnt platform wrapped
			//	- apparently we only want two source files from "src/thread/generic" so we cant glob that either
			// so we have to include every windows subfolder manually whilst making sure to exclude only that one file
			// annoying
			"src/audio/directsound/*.c",
			"src/audio/wasapi/*.c",
			"src/core/windows/*.c",
			"src/core/windows/*.cpp",
			"src/dialog/windows/*.c",
			"src/filesystem/windows/*.c",
			"src/haptic/windows/*.c",
			"src/hidapi/windows/hid.c",
			"src/io/windows/*.c",
			"src/joystick/gdk/*.c",
			"src/joystick/gdk/*.cpp",
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
			"src/video/windows/*.c",
			"src/video/windows/*.cpp",
#elif defined( __linux__ )
			"src/render/gpu/*.c",
			"src/render/opengl/*.c",
			"src/render/opengles2/*.c",
			"src/render/vulkan/*.c",
#endif
			NULL
		},
		.defines = (const char *[]) {
			"DLL_EXPORT",
#ifdef _WIN32
			"SDL_PLATFORM_WIN32",
			"HAVE_MODF",
#elif defined( __linux__ )
#endif
			NULL
		},
		.additionalIncludes = (const char *[]) {
			"src",	// this feels dirty, are we sure we want to do this?
			"include",
			"include/build_config",
			NULL
		},
		.additionalLibs = (const char *[]) {
#if defined( _WIN32 )
			"Ole32",
			"OleAut32",
			"Winmm",
			"Imm32",
			"Advapi32",
			"Shell32",
			"Cfgmgr32",
			"Gdi32",
			"SetupAPI",
			"Version",
			"user32",
#elif defined( __linux__ )
#endif
			NULL
		}
	};

	BuildConfig demo = {
		.name				= "demo",
		.dependsOn			= (BuildConfig *[]) {
			&sdl,
			NULL
		},
		.binaryName			= "sdl-demo-app",
		.binaryFolder		= sdlBinaryFolder,
		.sourceFiles		= (const char *[]) { "demo-app/*.cpp", NULL },
		.additionalIncludes	= (const char *[]) { "include", NULL },
		.additionalLibPaths	= (const char *[]) { sdlBinaryFolder, NULL },
		.additionalLibs		= (const char *[]) { sdlBinaryName, NULL },
		.warningsAsErrors	= true,
	};

	AddBuildConfig( &options, &demo );

	return Build( &options );
}
