#define BUILDER_IMPLEMENTATION
#include "../../builder.h"

#include "../test_compiler_override.h"

#define BINARY_NAME		"SDL"
#define BINARY_FOLDER	"bin"

#if defined( __linux__ )
#define WAYLAND_PROTOCOLS_FOLDER	"intermediate/wayland-generated-protocols"

// SDL's wayland backend includes client headers and links protocol glue that wayland-scanner generates from wayland-protocols/*.xml
static void GenerateWaylandProtocols( BuildConfig *config ) {
	(void) config;

	const char *cmd =
		"mkdir -p " WAYLAND_PROTOCOLS_FOLDER " && "
		"for xml in wayland-protocols/*.xml; do "
			"name=$(basename \"$xml\" .xml); "
			"h=" WAYLAND_PROTOCOLS_FOLDER "/$name-client-protocol.h; "
			"c=" WAYLAND_PROTOCOLS_FOLDER "/$name-protocol.c; "
			"[ \"$h\" -nt \"$xml\" ] || wayland-scanner client-header \"$xml\" \"$h\" || exit 1; "
			"[ \"$c\" -nt \"$xml\" ] || wayland-scanner private-code \"$xml\" \"$c\" || exit 1; "
		"done";

	if ( system( cmd ) != 0 ) {
		fprintf( stderr, "Failed to generate wayland protocols.  Is wayland-scanner installed?\n" );
		exit( 1 );
	}
}
#endif

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };
	ApplyCompilerOverride( &options, argc, argv );

	options.selfRebuildConfig = CreateBuildConfig( &options );
	*options.selfRebuildConfig = (BuildConfig) {
		.sourceFiles	= MakeStringList( "build.c" ),
	};

	// the cross-platform half goes in the initialiser; the per-platform half is appended below, because a #if inside a
	// macro argument list is undefined behaviour (clang's -Wembedded-directive) even though it happens to work
	BuildConfig *sdl = CreateBuildConfig( &options );
	*sdl = (BuildConfig) {
		.name			= "sdl",
		.binaryType		= BINARY_TYPE_DYNAMIC_LIBRARY,
		.binaryName		= BINARY_NAME,
		.binaryFolder	= BINARY_FOLDER,
		.sourceFiles	= MakeStringList(
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
			"src/video/yuv2rgb/*.c"
		),
		.defines			= MakeStringList( "DLL_EXPORT" ),
		.additionalIncludes	= MakeStringList(
			"src",	// this feels dirty, are we sure we want to do this?
			"include"
		),
	};

#if defined( _WIN32 )
	AddIncludes( sdl, "include/build_config" );

	// TODO(DM): 14/06/2025: we cant just do "src/**/windows/*.c" here because
	//	- "hidapi/windows/hid.c" includes "hidapi_descriptor_reconstruct.c" which we dont want to use on windows and it isnt platform wrapped
	//	- apparently we only want two source files from "src/thread/generic" so we cant glob that either
	// so we have to include every windows subfolder manually whilst making sure to exclude only that one file
	// annoying
	AddSourceFiles( sdl,
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
		"src/video/windows/*.cpp"
	);

	AddDefines( sdl, "SDL_PLATFORM_WIN32", "HAVE_MODF" );

	AddLibs( sdl,
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
		"user32"
	);
#elif defined( __linux__ )
	// SDL ships no hand-written linux build config, so build_config_linux/SDL_build_config.h is the one SDL's cmake generates
	// configured with x11 + wayland (both dlopen'd at runtime, so only headers needed at build time) and no external audio/dbus/udev/libusb deps
	// the source list below mirrors what that cmake config compiles
	AddIncludes( sdl, "build_config_linux" );

	AddSourceFiles( sdl,
		"src/camera/v4l2/*.c",
		"src/core/linux/SDL_evdev.c",
		"src/core/linux/SDL_evdev_capabilities.c",
		"src/core/linux/SDL_evdev_kbd.c",
		"src/core/linux/SDL_threadprio.c",
		"src/core/unix/*.c",
		"src/dialog/unix/*.c",
		"src/filesystem/posix/*.c",
		"src/filesystem/unix/*.c",
		"src/gpu/vulkan/*.c",
		"src/haptic/linux/*.c",
		"src/joystick/linux/*.c",
		"src/libm/*.c",
		"src/loadso/dlopen/*.c",
		"src/locale/unix/*.c",
		"src/misc/unix/*.c",
		"src/power/linux/*.c",
		"src/process/posix/*.c",
		"src/render/gpu/*.c",
		"src/render/opengl/*.c",
		"src/render/opengles2/*.c",
		"src/render/vulkan/*.c",
		"src/sensor/dummy/*.c",
		"src/storage/steam/*.c",
		"src/thread/pthread/*.c",
		"src/time/unix/*.c",
		"src/timer/unix/*.c",
		"src/tray/unix/*.c",
		"src/video/x11/*.c"
	);

	// only build the wayland backend if its headers and wayland-scanner are installed
	// x11-only machines skip it entirely and SDL falls back to x11
	bool hasWayland = system( "pkg-config --exists wayland-client wayland-cursor wayland-egl xkbcommon && command -v wayland-scanner > /dev/null" ) == 0;

	if ( hasWayland ) {
		AddDefines( sdl, "SDL_VIDEO_DRIVER_WAYLAND" );
		AddIncludes( sdl, WAYLAND_PROTOCOLS_FOLDER );
		AddSourceFiles( sdl, "src/video/wayland/*.c", WAYLAND_PROTOCOLS_FOLDER "/*.c" );

		sdl->OnPreBuild = GenerateWaylandProtocols;
	}

	// not AddLibs() - on linux builder turns bare lib names into "-l:<name>.so" which doesn't resolve system libs like libm
	AddLinkerArguments( sdl, "-lm" );
#endif

	if ( HasCommandLineArg( argc, argv, "--gcc" ) ) {
		// SDL_egl.h only falls back to its own built-in EGL/GLES declarations when _MSC_VER is defined;
		// GCC needs the real Khronos headers vendored here
		AddIncludes( sdl, "src/video/khronos" );

#if defined( _WIN32 )
		// MSVC/clang pull GUID_NULL, IID_IShellItem, IID_ITaskbarList3 etc in via their default libs;
		// MinGW needs libuuid.a linked explicitly to define them
		AddLibs( sdl, "uuid" );
#endif
	}

	BuildConfig *demo = CreateBuildConfig( &options );
	*demo = (BuildConfig) {
		.name				= "demo",
		.binaryType			= BINARY_TYPE_EXE,
		.binaryName			= "sdl-demo-app",
		.binaryFolder		= BINARY_FOLDER,
		.warningsAsErrors	= true,
		.dependsOn			= MakeDependencies( sdl ),
		.sourceFiles		= MakeStringList( "demo-app/*.cpp" ),
		.additionalIncludes	= MakeStringList( "include" ),
		.additionalLibPaths	= MakeStringList( BINARY_FOLDER ),
		.additionalLibs		= MakeStringList( BINARY_NAME ),
	};

	options.defaultConfig = demo;

	return Build( &options, argc, argv );
}
