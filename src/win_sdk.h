#pragma once

#include "core/include/core_string.h"
#include "core/include/core_types.h"

struct windowsSDKVersion_t {
	s32	v0, v1, v2, v3;
};

struct windowsSDK_t {
	String				ucrtInclude;
	String				umInclude;
	String				sharedInclude;

	String				ucrtLibPath;
	String				umLibPath;

	windowsSDKVersion_t	version;
};

bool8					Win_GetSDK( windowsSDK_t *outSDK );
