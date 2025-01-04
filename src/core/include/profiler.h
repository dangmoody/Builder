/*
===========================================================================

The Engine

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#pragma once

#define PROFILER_FRAME_MARKER()			profiler_frame_marker_internal()
#define PROFILER_SCOPE()				profiler_scope_named_internal( __FUNCTION__ )
#define PROFILER_SCOPE_NAMED( name )	profiler_scope_named_internal( name )

void profiler_frame_marker_internal();
void profiler_scope_named_internal( const char* name );
