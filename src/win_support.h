#pragma once

#ifdef _WIN32

#include "int_types.h"
#include "string.h"

struct linearAllocator_t;

struct windowsSDKVersion_t {
	s32	v0, v1, v2, v3;
};

struct windowsSDK_t {
	string_t				rootFolder;

	string_t				ucrtInclude;
	string_t				umInclude;
	string_t				sharedInclude;

	string_t				ucrtLibPath;
	string_t				umLibPath;

	windowsSDKVersion_t	version;
};

bool8	Win_GetWindowsSDK( linearAllocator_t *allocator, windowsSDK_t *outSDK );


//================================================================


struct msvcVersion_t {
	s32	v0, v1, v2;
};

struct msvcInstall_t {
	string_t			rootFolder;
	string_t			includePath;
	string_t			libPath;

	msvcVersion_t	version;
};

bool8	Win_GetMSVCInstall( linearAllocator_t *allocator, msvcInstall_t *outMSVC );

#endif // _WIN32
