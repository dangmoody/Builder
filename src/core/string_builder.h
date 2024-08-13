/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#pragma once

struct StringBuilderBuffer {
	char*					data;
	StringBuilderBuffer*	next;
};

struct StringBuilder {
	StringBuilderBuffer*	head;
	StringBuilderBuffer*	tail;
};

void						string_builder_reset( StringBuilder* builder );
void						string_builder_destroy( StringBuilder* builder );

void						string_builder_appendfv( StringBuilder* builder, const char* fmt, va_list args );
void						string_builder_appendf( StringBuilder* builder, const char* fmt, ... );

const char*					string_builder_to_string( StringBuilder* builder );