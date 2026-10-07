// SDL requires one of these when building
// windows doesnt need this at all, but linux definitely does
// SDL's cmake generates this, but for the purposes of a demo we are configuring and maintaining the most minimal version possible

#pragma once

#include <SDL3/SDL_platform_defines.h>

// libc headers
#define HAVE_STDDEF_H 1
#define HAVE_MATH_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_LINUX_INPUT_H 1

// libc functions
#define HAVE_GETENV 1
#define HAVE_SETENV 1
#define HAVE_UNSETENV 1
#define HAVE_SIGTIMEDWAIT 1
#define HAVE_GETRESUID 1
#define HAVE_GETRESGID 1

// platform backends
#define SDL_INPUT_LINUXEV 1
#define SDL_HAPTIC_LINUX 1
#define SDL_PROCESS_POSIX 1
#define SDL_LOADSO_DLOPEN 1
#define SDL_THREAD_PTHREAD 1
#define SDL_THREAD_PTHREAD_RECURSIVE_MUTEX 1
#define SDL_TIME_UNIX 1
#define SDL_TIMER_UNIX 1
#define SDL_FILESYSTEM_UNIX 1
#define SDL_FSOPS_POSIX 1

// wayland
// SDL_VIDEO_DRIVER_WAYLAND is defined by build.c, only when wayland is installed
#define SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC "libwayland-client.so.0"
#define SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_CURSOR "libwayland-cursor.so.0"
#define SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_EGL "libwayland-egl.so.1"
#define SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_LIBDECOR "libdecor-0.so.0"
#define SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_XKBCOMMON "libxkbcommon.so.0"

// x11
#define SDL_VIDEO_DRIVER_X11 1
#define SDL_VIDEO_DRIVER_X11_DYNAMIC "libX11.so.6"
#define SDL_VIDEO_DRIVER_X11_DYNAMIC_XTEST "libXtst.so.6"
#define SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS 1

// rendering
#define SDL_VIDEO_RENDER_OGL_ES2 1
#define SDL_VIDEO_OPENGL_ES2 1
#define SDL_VIDEO_OPENGL_GLX 1
#define SDL_VIDEO_OPENGL_EGL 1
#define SDL_VIDEO_VULKAN 1
