#pragma once

#ifdef _WIN32

#include "core/include/core_types.h"
#include "core/include/core_string.h"

struct windowsSDKVersion_t {
	s32	v0, v1, v2, v3;
};

struct windowsSDK_t {
	String				rootFolder;

	String				ucrtInclude;
	String				umInclude;
	String				sharedInclude;

	String				ucrtLibPath;
	String				umLibPath;

	windowsSDKVersion_t	version;
};

bool8	Win_GetWindowsSDK( windowsSDK_t *outSDK );


//================================================================


struct msvcVersion_t {
	s32	v0, v1, v2;
};

struct msvcInstall_t {
	String			rootFolder;
	String			includePath;
	String			libPath;

	msvcVersion_t	version;
};

bool8	Win_GetMSVCInstall( msvcInstall_t *outMSVC );

#endif // _WIN32
