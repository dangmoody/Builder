/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#include "../core.cpp"

#define TEMPER_IMPLEMENTATION
#define TEMPERDEV_ASSERT assert
#define NOMINMAX
#include <temper/temper.h>

#include <float.h>	// FLT_MAX


/*
================================================================================================

	IntAndFloat Types

================================================================================================
*/

//static bool8 doubleeq( const float64 lhs, const float64 rhs ) {
//	return fabs( lhs - rhs ) < 1e-5;
//}

TEMPER_TEST( number_types_sizes, TEMPER_FLAG_SHOULD_RUN ) {
	TEMPER_CHECK_TRUE( sizeof( s8 )  == 1 );
	TEMPER_CHECK_TRUE( sizeof( s16 ) == 2 );
	TEMPER_CHECK_TRUE( sizeof( s32 ) == 4 );
	TEMPER_CHECK_TRUE( sizeof( s64 ) == 8 );

	TEMPER_CHECK_TRUE( sizeof( u8 )  == 1 );
	TEMPER_CHECK_TRUE( sizeof( u16 ) == 2 );
	TEMPER_CHECK_TRUE( sizeof( u32 ) == 4 );
	TEMPER_CHECK_TRUE( sizeof( u64 ) == 8 );

	TEMPER_CHECK_TRUE( sizeof( float32 ) == 4 );
	TEMPER_CHECK_TRUE( sizeof( float64 ) == 8 );
}

TEMPER_TEST( number_types_ranges, TEMPER_FLAG_SHOULD_RUN ) {
	TEMPER_CHECK_TRUE( S8_MIN  == ( -127 - 1 ) );
	TEMPER_CHECK_TRUE( S16_MIN == ( -32767 - 1 ) );
	TEMPER_CHECK_TRUE( S32_MIN == ( -2147483647 - 1 ) );
	TEMPER_CHECK_TRUE( S64_MIN == ( -9223372036854775807 - 1 ) );

	TEMPER_CHECK_TRUE( S8_MAX  == 127 );
	TEMPER_CHECK_TRUE( S16_MAX == 32767 );
	TEMPER_CHECK_TRUE( S32_MAX == 2147483647 );
	TEMPER_CHECK_TRUE( S64_MAX == 9223372036854775807 );

	// TODO(DM): FLOAT32_MIN, FLOAT32_MAX, FLOAT64_MIN, FLOAT64_MAX
}


/*
================================================================================================

	Logging

================================================================================================
*/

TEMPER_TEST( print_functions, TEMPER_FLAG_SHOULD_SKIP ) {
	printf( "Test printf() call.  Please ignore.\n" );
	warning( "Test warning() call.  Please ignore.\n" );
	error( "Test error() call.  Please ignore.\n" );

	printf( "\n" );
}


/*
================================================================================================

	String Helpers

================================================================================================
*/

TEMPER_TEST( test_string_equals, TEMPER_FLAG_SHOULD_RUN ) {
	const char* string_a = "This is a string";
	const char* string_b = "This is a string";

	const char* string_c = "tHiS iS a StRiNg";
	const char* string_d = "hoolaba)";

	TEMPER_CHECK_TRUE(  string_equals( string_a, string_b ) );
	TEMPER_CHECK_TRUE( !string_equals( string_a, string_c ) );
	TEMPER_CHECK_TRUE( !string_equals( string_a, string_d ) );
	TEMPER_CHECK_TRUE( !string_equals( string_b, string_c ) );
	TEMPER_CHECK_TRUE( !string_equals( string_b, string_d ) );
	TEMPER_CHECK_TRUE( !string_equals( string_c, string_d ) );
}

TEMPER_PARAMETRIC( test_string_starts_with, TEMPER_FLAG_SHOULD_RUN, const char* string, const char* prefix ) {
	TEMPER_CHECK_TRUE( string_starts_with( string, prefix ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "There are 69,105 leaves on the pile.", "T" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "There are 69,105 leaves on the pile.", "Th" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "There are 69,105 leaves on the pile.", "There" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "There are 69,105 leaves on the pile.", "There " );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "There are 69,105 leaves on the pile.", "There are" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "There are 69,105 leaves on the pile.", "There are " );

TEMPER_PARAMETRIC( test_string_ends_with, TEMPER_FLAG_SHOULD_RUN, const char* string, const char* prefix ) {
	TEMPER_CHECK_TRUE( string_ends_with( string, prefix ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with, "I'm going to be as forthcoming as I can be, Mr Anderson.", ", Mr Anderson." );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with, "I'm going to be as forthcoming as I can be, Mr Anderson.", "Anderson." );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with, "I'm going to be as forthcoming as I can be, Mr Anderson.", "son." );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with, "I'm going to be as forthcoming as I can be, Mr Anderson.", "." );

TEMPER_PARAMETRIC( test_string_contains, TEMPER_FLAG_SHOULD_RUN, const char* string, const char* substring ) {
	TEMPER_CHECK_TRUE( string_contains( string, substring ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", "I'm" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", "I'm " );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", "to be as" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", "to be as " );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", " to be as" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", " to be as " );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", "forthcoming" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", " as I can be" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", " " );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", ", Mr Anderson." );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", "Anderson." );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", "son." );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", "son" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "I'm going to be as forthcoming as I can be, Mr Anderson.", "." );

TEMPER_PARAMETRIC( test_string_substring, TEMPER_FLAG_SHOULD_RUN, const char* string, const u64 start, const u64 count, const char* expected_substring ) {
	char* actual_susbtring = cast( char* ) alloca( count * sizeof( char ) );

	string_substring( string, start, count, actual_susbtring );

	TEMPER_CHECK_TRUE( string_equals( expected_substring, actual_susbtring ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_substring, "As you can see we've had our eye on you for some time now, Mr Anderson.", 0,  15, "As you can see" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_substring, "As you can see we've had our eye on you for some time now, Mr Anderson.", 15, 25, "we've had our eye on you" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_substring, "As you can see we've had our eye on you for some time now, Mr Anderson.", 40, 18, "for some time now" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_substring, "As you can see we've had our eye on you for some time now, Mr Anderson.", 57, 15, ", Mr Anderson." );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_substring, "As you can see we've had our eye on you for some time now, Mr Anderson.", 59, 12, "Mr Anderson" );


/*
================================================================================================

	AllocatorLinearData

================================================================================================
*/

struct TestStruct {
	u32			x;
	const char*	string;
};

TEMPER_PARAMETRIC( linear_allocator_create, TEMPER_FLAG_SHOULD_RUN, AllocatorLinearData** allocator, const u64 size_bytes ) {
	TEMPER_CHECK_TRUE( allocator );
	TEMPER_CHECK_TRUE( !*allocator );

	mem_create_linear( size_bytes, cast( void** ) allocator );

	TEMPER_CHECK_TRUE( ( *allocator )->offset == 0 );
	TEMPER_CHECK_TRUE( ( *allocator )->size_bytes == size_bytes );
}

TEMPER_PARAMETRIC( linear_allocator_allocate, TEMPER_FLAG_SHOULD_RUN, AllocatorLinearData** allocator, const u32 x, const char* string ) {
	TEMPER_CHECK_TRUE( allocator );
	TEMPER_CHECK_TRUE( *allocator );

	u64 old_offset = ( *allocator )->offset;
	u64 old_size = ( *allocator )->size_bytes;

	TEMPER_CHECK_TRUE( ( *allocator )->offset == old_offset );
	TEMPER_CHECK_TRUE( ( *allocator )->size_bytes == old_size );

	TestStruct* test_struct = cast( TestStruct* ) mem_alloc_linear( *allocator, sizeof( TestStruct ), __FILE__, __LINE__ );
	test_struct->x = x;
	test_struct->string = string;

	TEMPER_CHECK_TRUE( test_struct );
	TEMPER_CHECK_TRUE( test_struct->x == x );
	TEMPER_CHECK_TRUE( string_equals( test_struct->string, string ) );

	u64 expected_new_offset = align_up( old_offset + sizeof( TestStruct ) + sizeof( LinearAllocatorHeader ), cast( u64 ) MEMORY_ALIGNMENT_EIGHT );

	TEMPER_CHECK_TRUE( ( *allocator )->offset == expected_new_offset );
	TEMPER_CHECK_TRUE( ( *allocator )->offset != old_offset );
	TEMPER_CHECK_TRUE( ( *allocator )->size_bytes == old_size );
}

TEMPER_PARAMETRIC( linear_allocator_reset, TEMPER_FLAG_SHOULD_RUN, AllocatorLinearData** allocator ) {
	TEMPER_CHECK_TRUE( allocator );
	TEMPER_CHECK_TRUE( *allocator );

	u64 old_offset = ( *allocator )->offset;
	u64 old_size = ( *allocator )->size_bytes;

	TEMPER_CHECK_TRUE( ( *allocator )->offset == old_offset );
	TEMPER_CHECK_TRUE( ( *allocator )->size_bytes == old_size );

	mem_reset_linear( *allocator );

	TEMPER_CHECK_TRUE( ( *allocator )->offset == 0 );
	TEMPER_CHECK_TRUE( ( *allocator )->offset != old_offset );
	TEMPER_CHECK_TRUE( ( *allocator )->size_bytes == old_size );
}

TEMPER_PARAMETRIC( linear_allocator_destroy, TEMPER_FLAG_SHOULD_RUN, AllocatorLinearData** allocator ) {
	TEMPER_CHECK_TRUE( allocator );
	TEMPER_CHECK_TRUE( *allocator );

	mem_destroy_linear( *allocator );
	allocator = NULL;

	TEMPER_CHECK_TRUE( !allocator );
}

static AllocatorLinearData* g_allocator = NULL;

TEMPER_INVOKE_PARAMETRIC_TEST( linear_allocator_create, &g_allocator, MEM_KILOBYTES( 1 ) );

TEMPER_INVOKE_PARAMETRIC_TEST( linear_allocator_allocate, &g_allocator, 100, "this is a test string" );
TEMPER_INVOKE_PARAMETRIC_TEST( linear_allocator_allocate, &g_allocator, 200, "this is another test string" );
TEMPER_INVOKE_PARAMETRIC_TEST( linear_allocator_allocate, &g_allocator, 300, "and another one" );
TEMPER_INVOKE_PARAMETRIC_TEST( linear_allocator_allocate, &g_allocator, 400, "one last time" );
TEMPER_INVOKE_PARAMETRIC_TEST( linear_allocator_allocate, &g_allocator, 500, "ok im done now" );

TEMPER_INVOKE_PARAMETRIC_TEST( linear_allocator_reset, &g_allocator );

TEMPER_INVOKE_PARAMETRIC_TEST( linear_allocator_allocate, &g_allocator, 555, "should be starting from the beginning again now" );
TEMPER_INVOKE_PARAMETRIC_TEST( linear_allocator_allocate, &g_allocator, 666, "your message here" );
TEMPER_INVOKE_PARAMETRIC_TEST( linear_allocator_allocate, &g_allocator, 777, "dont look directly into the bugs" );
TEMPER_INVOKE_PARAMETRIC_TEST( linear_allocator_allocate, &g_allocator, 888, "rzcore is great" );
TEMPER_INVOKE_PARAMETRIC_TEST( linear_allocator_allocate, &g_allocator, 999, "Ross Kemp" );

TEMPER_INVOKE_PARAMETRIC_TEST( linear_allocator_destroy, &g_allocator );


/*
================================================================================================

	AllocatorGeneric

================================================================================================
*/

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-parameter"

TEMPER_PARAMETRIC( generic_allocator_create, TEMPER_FLAG_SHOULD_RUN, AllocatorGeneric** allocator, const u64 size ) {
	TEMPER_CHECK_TRUE( allocator );
	TEMPER_CHECK_TRUE( !*allocator );

	mem_create_generic( size, cast( void** ) allocator );

	TEMPER_CHECK_TRUE( ( *allocator )->tail == NULL );
}

TEMPER_PARAMETRIC( generic_allocator_destroy, TEMPER_FLAG_SHOULD_RUN, AllocatorGeneric** allocator ) {
	TEMPER_CHECK_TRUE( ( *allocator )->tail == NULL );

	mem_destroy_generic( *allocator );
	( *allocator ) = NULL;

	TEMPER_CHECK_TRUE( !*allocator );
}

TEMPER_PARAMETRIC( generic_allocator_allocate, TEMPER_FLAG_SHOULD_RUN, AllocatorGeneric** allocator, const TestStruct thing, TestStruct** out_thing ) {
	GenericAllocationHeader* previous_tail = ( *allocator )->tail;

	// DM: 23/12/2022: obviously this is imperfect
	// but idk how else to elegantly automate getting the line that the TestStruct got allocated at
	const u64 alloc_line = __LINE__ + 2;

	TestStruct* alloced_thing = cast( TestStruct* ) mem_alloc_generic( ( *allocator ), sizeof( TestStruct ), __FILE__, __LINE__ );
	alloced_thing->x = thing.x;
	alloced_thing->string = thing.string;

	*out_thing = alloced_thing;

	TEMPER_CHECK_TRUE( alloced_thing != NULL );
	TEMPER_CHECK_TRUE( alloced_thing->x == thing.x );
	TEMPER_CHECK_TRUE( string_equals( alloced_thing->string, thing.string ) );

	// check header
	{
		u8* ptr = cast( u8* ) alloced_thing;

		TEMPER_CHECK_TRUE( ( *allocator )->tail == cast( GenericAllocationHeader* ) ptr - 1 );
		TEMPER_CHECK_TRUE( ( *allocator )->tail->next == NULL );
		TEMPER_CHECK_TRUE( ( *allocator )->tail->prev == previous_tail );
		TEMPER_CHECK_TRUE( ( *allocator )->tail->ptr == alloced_thing );
		TEMPER_CHECK_TRUE( memcmp( ( *allocator )->tail->ptr, alloced_thing, sizeof( TestStruct ) ) == 0 );
		TEMPER_CHECK_TRUE( ( *allocator )->tail->size == sizeof( TestStruct ) );
		TEMPER_CHECK_TRUE( ( *allocator )->tail->line == alloc_line );
		TEMPER_CHECK_TRUE( string_equals( ( *allocator )->tail->file, __FILE__ ) );
	}
}

#pragma clang diagnostic pop

TEMPER_PARAMETRIC( generic_allocator_free, TEMPER_FLAG_SHOULD_RUN, AllocatorGeneric** allocator, TestStruct* thing, const bool8 freeing_tail, const bool8 all_things_are_free ) {
	GenericAllocationHeader* old_tail = ( *allocator )->tail;
	GenericAllocationHeader* old_tail_prev = ( *allocator )->tail->prev;

	TestStruct thing_copy = {
		.x		= thing->x,
		.string	= thing->string
	};

	mem_free_generic( ( *allocator ), thing, __FILE__, __LINE__ );

	TEMPER_CHECK_TRUE( thing->x != thing_copy.x );
	TEMPER_CHECK_TRUE( thing->string != thing_copy.string );

	thing = NULL;

	if ( freeing_tail ) {
		TEMPER_CHECK_TRUE( ( *allocator )->tail != old_tail );
		TEMPER_CHECK_TRUE( ( *allocator )->tail == old_tail_prev );
	}

	// are all things free?
	{
		const bool8 actual_answer = ( *allocator )->tail == NULL;

		TEMPER_CHECK_TRUE( actual_answer == all_things_are_free );
	}
}

static AllocatorGeneric* g_allocator_generic = NULL;

static TestStruct* g_thing0 = NULL;
static TestStruct* g_thing1 = NULL;
static TestStruct* g_thing2 = NULL;
static TestStruct* g_thing3 = NULL;
static TestStruct* g_thing4 = NULL;

TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_create, &g_allocator_generic, MEM_KILOBYTES( 1 ) );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_destroy, &g_allocator_generic );

TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_create, &g_allocator_generic, MEM_KILOBYTES( 1 ) );

TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_allocate, &g_allocator_generic, { 5,  "This is a string"       }, &g_thing0 );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_allocate, &g_allocator_generic, { 6,  "This is another string" }, &g_thing1 );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_allocate, &g_allocator_generic, { 9,  "Did this get malloced?" }, &g_thing2 );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_allocate, &g_allocator_generic, { 14, "What about this one?"   }, &g_thing3 );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_allocate, &g_allocator_generic, { 19, "Did it really though?"  }, &g_thing4 );

// first test the ideal use-case of free-ing in reverse order
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_free, &g_allocator_generic, g_thing4, true, false );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_free, &g_allocator_generic, g_thing3, true, false );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_free, &g_allocator_generic, g_thing2, true, false );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_free, &g_allocator_generic, g_thing1, true, false );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_free, &g_allocator_generic, g_thing0, true, true );

// now test out of order freeing
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_allocate, &g_allocator_generic, { 1,  "one"     }, &g_thing0 );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_allocate, &g_allocator_generic, { 2,  "two"     }, &g_thing1 );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_allocate, &g_allocator_generic, { 4,  "four"    }, &g_thing2 );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_allocate, &g_allocator_generic, { 8,  "eight"   }, &g_thing3 );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_allocate, &g_allocator_generic, { 16, "sixteen" }, &g_thing4 );

TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_free, &g_allocator_generic, g_thing0, false, false );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_free, &g_allocator_generic, g_thing1, false, false );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_free, &g_allocator_generic, g_thing3, false, false );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_free, &g_allocator_generic, g_thing4, true, false );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_free, &g_allocator_generic, g_thing2, true, true );

TEMPER_PARAMETRIC( generic_allocator_realloc, TEMPER_FLAG_SHOULD_RUN, AllocatorGeneric** allocator, TestStruct** out_things_array, u64* out_things_array_count, const u64 num_things_to_add ) {
	TEMPER_CHECK_TRUE( out_things_array_count );

	// cache things before realloc call
	u64 old_count = ( *out_things_array_count );
	u8* old_ptr = cast( u8* ) *out_things_array;
	GenericAllocationHeader* old_header = old_ptr ? ( cast( GenericAllocationHeader* ) old_ptr - 1 ) : NULL;
	GenericAllocationHeader* old_prev = old_header ? old_header->prev : NULL;
	GenericAllocationHeader* old_next = old_header ? old_header->next : NULL;

	*out_things_array_count += num_things_to_add;

	u64 new_realloc_size = ( *out_things_array_count ) * sizeof( TestStruct );

	u64 alloc_line = __LINE__ + 2;

	*out_things_array = cast( TestStruct* ) mem_realloc_generic( *allocator, *out_things_array, new_realloc_size, __FILE__, __LINE__ );

	// get new values after realloc call
	u8* ptr = cast( u8* ) *out_things_array;
	GenericAllocationHeader* header = cast( GenericAllocationHeader* ) ptr - 1;

	TEMPER_CHECK_TRUE( ( *out_things_array ) != NULL );
	TEMPER_CHECK_TRUE( ( *out_things_array_count ) == old_count + num_things_to_add );

	// check header
	{
		TEMPER_CHECK_TRUE( header->ptr == ( *out_things_array ) );
		TEMPER_CHECK_TRUE( header->size == new_realloc_size );
		TEMPER_CHECK_TRUE( string_equals( header->file, __FILE__ ) );
		TEMPER_CHECK_TRUE( header->line == alloc_line );
		TEMPER_CHECK_TRUE( header->prev == old_prev );
		TEMPER_CHECK_TRUE( header->next == old_next );

		if ( header->prev ) {
			TEMPER_CHECK_TRUE( header->prev->next == header );
		}

		if ( header->next ) {
			TEMPER_CHECK_TRUE( header->next->prev == header );
		}
	}
}

static u64 things_array_count = 0;
static TestStruct* things_array = NULL;

TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_realloc, &g_allocator_generic, &things_array, &things_array_count, 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_realloc, &g_allocator_generic, &things_array, &things_array_count, 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_realloc, &g_allocator_generic, &things_array, &things_array_count, 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_realloc, &g_allocator_generic, &things_array, &things_array_count, 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_realloc, &g_allocator_generic, &things_array, &things_array_count, 1 );

TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_free, &g_allocator_generic, things_array, true, true );

TEMPER_INVOKE_PARAMETRIC_TEST( generic_allocator_destroy, &g_allocator_generic );


/*
================================================================================================

	Array

================================================================================================
*/

static Array<s32> g_test_array;
static Array<s32> g_test_array_copy;

TEMPER_PARAMETRIC( array_get_element_at_index, TEMPER_FLAG_SHOULD_RUN, Array<s32>* array, const u64 index, const s32 expected_element ) {
	TEMPER_CHECK_TRUE( (*array)[index] == expected_element );
}

TEMPER_PARAMETRIC( test_array_zero, TEMPER_FLAG_SHOULD_RUN, Array<s32>* out_array ) {
	array_zero( out_array );

	TEMPER_CHECK_TRUE( out_array->data == NULL );
	TEMPER_CHECK_TRUE( out_array->count == 0 );
	TEMPER_CHECK_TRUE( out_array->alloced == 0 );
}

TEMPER_PARAMETRIC( test_array_add, TEMPER_FLAG_SHOULD_RUN, Array<s32>* array, s32 element ) {
	u64 old_count = array->count;
	u64 old_alloced = array->alloced;

	array_add( array, element );

	TEMPER_CHECK_TRUE( array->data[array->count - 1] == element );

	TEMPER_CHECK_TRUE( array->data != NULL );
	TEMPER_CHECK_TRUE( array->count == old_count + 1 );

	u64 new_alloced = 0;
	if ( old_alloced == 0 ) {
		new_alloced = 1;
	} else {
		new_alloced = old_alloced << 1;
	}

	if ( old_count == old_alloced ) {
		TEMPER_CHECK_TRUE( array->alloced == new_alloced );
	}
}

TEMPER_PARAMETRIC( test_array_reset, TEMPER_FLAG_SHOULD_RUN, Array<s32>* array ) {
	array_reset( array );

	TEMPER_CHECK_TRUE( array->count == 0 );
}

TEMPER_PARAMETRIC( test_array_resize, TEMPER_FLAG_SHOULD_RUN, Array<s32>* array, const u64 new_count ) {
	u64 old_count = array->count;
	u64 old_alloced = array->alloced;

	array_resize( array, new_count );

	u64 new_alloced = 0;
	if ( new_count >= old_alloced ) {
		new_alloced = next_po2_up( new_count );
	} else {
		new_alloced = old_alloced;
	}

	TEMPER_CHECK_TRUE( array->count == new_count );
	TEMPER_CHECK_TRUE( array->alloced == new_alloced );
}

TEMPER_PARAMETRIC( test_array_reserve, TEMPER_FLAG_SHOULD_RUN, Array<s32>* array, const u64 new_count ) {
	u64 old_count = array->count;
	u64 old_alloced = array->alloced;

	array_reserve( array, new_count );

	TEMPER_CHECK_TRUE( array->count == old_count );

	if ( new_count > old_alloced ) {
		TEMPER_CHECK_TRUE( array->alloced == next_po2_up( new_count ) );
	} else {
		TEMPER_CHECK_TRUE( array->alloced == old_alloced );
	}
}

TEMPER_PARAMETRIC( test_array_copy, TEMPER_FLAG_SHOULD_RUN, Array<s32>* original_array, Array<s32>* new_array ) {
	array_copy( new_array, original_array );

	TEMPER_CHECK_TRUE( new_array->count == original_array->count );
	TEMPER_CHECK_TRUE( new_array->alloced == next_po2_up( new_array->count ) );

	For ( u64, i, 0, new_array->count ) {
		TEMPER_CHECK_TRUE_M( (*new_array)[i] == (*original_array)[i], "new_array[%llu] != original_array[%llu] when it should!", i, i );
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_array_zero, &g_test_array );

TEMPER_INVOKE_PARAMETRIC_TEST( test_array_add, &g_test_array, 123 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_array_add, &g_test_array, 69 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_array_add, &g_test_array, 420 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_array_add, &g_test_array, 666 );

TEMPER_INVOKE_PARAMETRIC_TEST( array_get_element_at_index, &g_test_array, 0, 123 );
TEMPER_INVOKE_PARAMETRIC_TEST( array_get_element_at_index, &g_test_array, 1, 69 );
TEMPER_INVOKE_PARAMETRIC_TEST( array_get_element_at_index, &g_test_array, 2, 420 );
TEMPER_INVOKE_PARAMETRIC_TEST( array_get_element_at_index, &g_test_array, 3, 666 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_array_reset, &g_test_array );

TEMPER_INVOKE_PARAMETRIC_TEST( test_array_resize, &g_test_array, 5 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_array_resize, &g_test_array, 10 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_array_resize, &g_test_array, 6 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_array_reserve, &g_test_array, 20 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_array_reserve, &g_test_array, 10 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_array_copy, &g_test_array, &g_test_array_copy );


/*
================================================================================================

	hash32 and hash64

================================================================================================
*/

TEMPER_PARAMETRIC( hash_string_equals_hash32, TEMPER_FLAG_SHOULD_RUN, const char* string, const u32 seed, const u32 expected_hash ) {
	const u64 string_length = strlen( string );

	TEMPER_CHECK_TRUE( hash32( string, string_length, seed ) == expected_hash );
}

TEMPER_PARAMETRIC( hash_string_equals_hash64, TEMPER_FLAG_SHOULD_RUN, const char* string, const u64 seed, const u64 expected_hash ) {
	const u64 string_length = strlen( string );

	TEMPER_CHECK_TRUE( hash64( string, string_length, seed ) == expected_hash );
}

TEMPER_INVOKE_PARAMETRIC_TEST( hash_string_equals_hash32, "test_hash_string_value", 0, 163121569U );
TEMPER_INVOKE_PARAMETRIC_TEST( hash_string_equals_hash64, "test_hash_string_value", 0, 17747826225912071899ULL );


/*
================================================================================================

	Hashmap32

================================================================================================
*/

TEMPER_PARAMETRIC( test_hashmap32_create, TEMPER_FLAG_SHOULD_RUN, Hashmap32** hashmap, const u32 count ) {
	TEMPER_CHECK_TRUE( hashmap );
	TEMPER_CHECK_TRUE( !*hashmap );

	TEMPER_CHECK_TRUE( count );

	*hashmap = hashmap32_create( count );

	TEMPER_CHECK_TRUE( ( *hashmap )->count == count );

	For ( u64, i, 0, ( *hashmap )->count ) {
		TEMPER_CHECK_TRUE_A( ( *hashmap )->keys[i] == HASHMAP32_UNUSED );
	}

	For ( u64, i, 0, ( *hashmap )->count ) {
		TEMPER_CHECK_TRUE_A( ( *hashmap )->values[i] == HASHMAP32_UNUSED );
	}
}

TEMPER_PARAMETRIC( test_hashmap32_set_and_get_value, TEMPER_FLAG_SHOULD_RUN, Hashmap32* hashmap, const char* name, const u32 age ) {
	TEMPER_CHECK_TRUE( hashmap );

	const u32 name_hash = hash32( name, strlen( name ), 0 );

	TEMPER_CHECK_TRUE( hashmap32_get_value( hashmap, name_hash ) == 0 );

	hashmap32_set_value( hashmap, name_hash, age );

	TEMPER_CHECK_TRUE( hashmap32_get_value( hashmap, name_hash ) == age );
	TEMPER_CHECK_TRUE( hashmap32_get_value( hashmap, name_hash ) != 0 );
}

TEMPER_PARAMETRIC( test_hashmap32_reset, TEMPER_FLAG_SHOULD_RUN, Hashmap32* hashmap ) {
	TEMPER_CHECK_TRUE( hashmap );

	u32 old_count = hashmap->count;

	hashmap32_reset( hashmap );

	TEMPER_CHECK_TRUE( hashmap->count == old_count );

	For ( u64, i, 0, hashmap->count ) {
		TEMPER_CHECK_TRUE_A( hashmap->keys[i] == HASHMAP32_UNUSED );
	}

	For ( u64, i, 0, hashmap->count ) {
		TEMPER_CHECK_TRUE_A( hashmap->values[i] == HASHMAP32_UNUSED );
	}
}

static Hashmap32* g_hashmap32 = NULL;

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap32_create, &g_hashmap32, 10 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap32_set_and_get_value, g_hashmap32, "Dan",     27 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap32_set_and_get_value, g_hashmap32, "Mum",     53 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap32_set_and_get_value, g_hashmap32, "Dad",     62 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap32_set_and_get_value, g_hashmap32, "Grandad", 89 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap32_reset, g_hashmap32 );


/*
================================================================================================

	Hashmap64

================================================================================================
*/

TEMPER_PARAMETRIC( test_hashmap64_create, TEMPER_FLAG_SHOULD_RUN, Hashmap64** hashmap, const u64 count ) {
	TEMPER_CHECK_TRUE( hashmap );
	TEMPER_CHECK_TRUE( !*hashmap );

	TEMPER_CHECK_TRUE( count );

	*hashmap = hashmap64_create( count );

	TEMPER_CHECK_TRUE( ( *hashmap )->count == count );

	For ( u64, i, 0, ( *hashmap )->count ) {
		TEMPER_CHECK_TRUE_A( ( *hashmap )->keys[i] == HASHMAP64_UNUSED );
	}

	For ( u64, i, 0, ( *hashmap )->count ) {
		TEMPER_CHECK_TRUE_A( ( *hashmap )->values[i] == HASHMAP64_UNUSED );
	}
}

TEMPER_PARAMETRIC( test_hashmap64_set_and_get_value, TEMPER_FLAG_SHOULD_RUN, Hashmap64* hashmap, const char* name, const u64 age ) {
	TEMPER_CHECK_TRUE( hashmap );

	const u64 name_hash = hash64( name, strlen( name ), 0 );

	TEMPER_CHECK_TRUE( hashmap64_get_value( hashmap, name_hash ) == 0 );

	hashmap64_set_value( hashmap, name_hash, age );

	TEMPER_CHECK_TRUE( hashmap64_get_value( hashmap, name_hash ) == age );
	TEMPER_CHECK_TRUE( hashmap64_get_value( hashmap, name_hash ) != 0 );
}

TEMPER_PARAMETRIC( test_hashmap64_reset, TEMPER_FLAG_SHOULD_RUN, Hashmap64* hashmap ) {
	TEMPER_CHECK_TRUE( hashmap );

	u64 old_count = hashmap->count;

	hashmap64_reset( hashmap );

	TEMPER_CHECK_TRUE( hashmap->count == old_count );

	For ( u64, i, 0, hashmap->count ) {
		TEMPER_CHECK_TRUE_A( hashmap->keys[i] == HASHMAP64_UNUSED );
	}

	For ( u64, i, 0, hashmap->count ) {
		TEMPER_CHECK_TRUE_A( hashmap->values[i] == HASHMAP64_UNUSED );
	}
}

static Hashmap64* g_hashmap64 = NULL;

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap64_create, &g_hashmap64, 10 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap64_set_and_get_value, g_hashmap64, "Dan",     27 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap64_set_and_get_value, g_hashmap64, "Mum",     53 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap64_set_and_get_value, g_hashmap64, "Dad",     62 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap64_set_and_get_value, g_hashmap64, "Grandad", 89 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap64_reset, g_hashmap64 );


/*
================================================================================================

	ring_t

================================================================================================
*/

enum {
	RING_SIZE = 10,
};

static Ring<TestStruct, RING_SIZE>	g_test_ring;

TEMPER_PARAMETRIC( test_ring_init, TEMPER_FLAG_SHOULD_RUN, Ring<TestStruct, RING_SIZE>* ring ) {
	ring_init( ring );

	TEMPER_CHECK_TRUE( ring->read_pos == 0 );
	TEMPER_CHECK_TRUE( ring->write_pos == 0 );
	TEMPER_CHECK_TRUE( ring->current_elements == 0 );

	TEMPER_CHECK_TRUE( ring->data.count == RING_SIZE + 1 );

	TEMPER_CHECK_TRUE( ring_empty( ring ) );
}

TEMPER_PARAMETRIC( test_ring_reset, TEMPER_FLAG_SHOULD_RUN, Ring<TestStruct, RING_SIZE>* ring ) {
	ring_reset( ring );

	TEMPER_CHECK_TRUE( ring->read_pos == 0 );
	TEMPER_CHECK_TRUE( ring->write_pos == 0 );
	TEMPER_CHECK_TRUE( ring->current_elements == 0 );

	TEMPER_CHECK_TRUE( ring_empty( ring ) );
}

TEMPER_PARAMETRIC( test_ring_push, TEMPER_FLAG_SHOULD_RUN, Ring<TestStruct, RING_SIZE>* ring, const TestStruct test_struct, const u32 expected_write_pos ) {
	u64 old_read_pos = ring->read_pos;
	u64 old_write_pos = ring->write_pos;

	ring_push( ring, test_struct );

	u64 new_read_pos = ring->read_pos;
	u64 new_write_pos = ring->write_pos;

	// ring should never be empty after pushing on to it
	TEMPER_CHECK_TRUE( !ring_empty( ring ) );

	TEMPER_CHECK_TRUE( new_read_pos == old_read_pos );
	TEMPER_CHECK_TRUE( new_write_pos != old_write_pos );
	TEMPER_CHECK_TRUE( new_write_pos == expected_write_pos );
}

TEMPER_PARAMETRIC( test_ring_pop, TEMPER_FLAG_SHOULD_RUN, Ring<TestStruct, RING_SIZE>* ring, const TestStruct expected_test_struct, const u32 expected_read_pos, const bool8 expected_empty ) {
	u64 old_read_pos = ring->read_pos;
	u64 old_write_pos = ring->write_pos;

	TestStruct test_struct = ring_pop( ring );

	TEMPER_CHECK_TRUE( test_struct.x == expected_test_struct.x );
	TEMPER_CHECK_TRUE( string_equals( test_struct.string, expected_test_struct.string ) );

	TEMPER_CHECK_TRUE( ring_empty( ring ) == expected_empty );

	u64 new_read_pos = ring->read_pos;
	u64 new_write_pos = ring->write_pos;

	TEMPER_CHECK_TRUE( new_read_pos != old_read_pos );
	TEMPER_CHECK_TRUE( new_write_pos == old_write_pos );
	TEMPER_CHECK_TRUE( new_read_pos == expected_read_pos );
}

TEMPER_PARAMETRIC( test_ring_empty, TEMPER_FLAG_SHOULD_RUN, Ring<TestStruct, RING_SIZE>* ring, const bool8 expected_empty ) {
	TEMPER_CHECK_TRUE( ring_empty( ring ) == expected_empty );
}

TEMPER_PARAMETRIC( test_ring_full, TEMPER_FLAG_SHOULD_RUN, Ring<TestStruct, RING_SIZE>* ring, const bool8 expected_full ) {
	bool8 is_full = ring_full( ring );
	TEMPER_CHECK_TRUE( is_full == expected_full );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_init,  &g_test_ring );

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_empty, &g_test_ring, true );	// should be empty given nothing has been added to it
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_full,  &g_test_ring, false );	// therefore should not be full

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 10, "test string" }, 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 11, "another test string" }, 2 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 12, "and again" }, 3 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 13, "and again" }, 4 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 14, "test string" }, 5 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 15, "test string" }, 6 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 16, "test string" }, 7 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 17, "test string" }, 8 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 18, "test string" }, 9 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 19, "test string" }, 10 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_empty, &g_test_ring, false );	// should not be empty after filling it up
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_full,  &g_test_ring, true );	// and should be full
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 10, "test string" }, 1, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 11, "another test string" }, 2, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 12, "and again" }, 3, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 13, "and again" }, 4, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 14, "test string" }, 5, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 15, "test string" }, 6, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 16, "test string" }, 7, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 17, "test string" }, 8, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 18, "test string" }, 9, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 19, "test string" }, 10, true );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_empty, &g_test_ring, true );

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 10, "test string" }, 0 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 11, "another test string" }, 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 12, "and again" }, 2 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 13, "and again" }, 3 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 14, "test string" }, 4 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 15, "test string" }, 5 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 16, "test string" }, 6 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 17, "test string" }, 7 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 18, "test string" }, 8 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 19, "test string" }, 9 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_empty, &g_test_ring, false );	// should not be empty after filling it up
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_full,  &g_test_ring, true );	// and should be full

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_reset, &g_test_ring );

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_empty, &g_test_ring, true );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_full,  &g_test_ring, false );


/*
================================================================================================

	File

================================================================================================
*/

TEMPER_PARAMETRIC( test_file_open_or_create, TEMPER_FLAG_SHOULD_RUN, File* file, const char* filename ) {
	TEMPER_CHECK_TRUE( filename );

	*file = file_open_or_create( filename );

	TEMPER_CHECK_TRUE( file->ptr );
	TEMPER_CHECK_TRUE( file->offset == 0 );
}

TEMPER_PARAMETRIC( test_file_open, TEMPER_FLAG_SHOULD_RUN, File* file, const char* filename, const bool8 should_exist ) {
	TEMPER_CHECK_TRUE( file );
	TEMPER_CHECK_TRUE( !file->ptr );

	*file = file_open( filename );

	bool8 file_exists = file->ptr != NULL;

	if ( should_exist ) {
		TEMPER_CHECK_TRUE( file->ptr != NULL );
	} else {
		TEMPER_CHECK_TRUE( file->ptr == NULL );
	}

	TEMPER_CHECK_TRUE( file_exists == should_exist );
	TEMPER_CHECK_TRUE( file->offset == 0 );
}

TEMPER_PARAMETRIC( test_file_close, TEMPER_FLAG_SHOULD_RUN, File* file ) {
	TEMPER_CHECK_TRUE( file );
	TEMPER_CHECK_TRUE( file->ptr );

	bool8 closed = file_close( file );

	TEMPER_CHECK_TRUE( closed );

	TEMPER_CHECK_TRUE( !file->ptr );
}

TEMPER_PARAMETRIC( test_file_write_entire, TEMPER_FLAG_SHOULD_RUN, const char* filename, const char* data_to_write ) {
	TEMPER_CHECK_TRUE( filename );
	TEMPER_CHECK_TRUE( data_to_write );

	u64 num_bytes_to_write = strlen( data_to_write );

	TEMPER_CHECK_TRUE( num_bytes_to_write );

	bool8 written = file_write_entire( filename, data_to_write, num_bytes_to_write );

	TEMPER_CHECK_TRUE( written == true );
}

TEMPER_PARAMETRIC( test_file_write, TEMPER_FLAG_SHOULD_RUN, File* file, const char* data_to_write, const u64 offset ) {
	TEMPER_CHECK_TRUE( file && file->ptr );
	TEMPER_CHECK_TRUE( data_to_write );

	u64 num_bytes_to_write = strlen( data_to_write );

	TEMPER_CHECK_TRUE( num_bytes_to_write );

	bool8 written = file_write( file, data_to_write, offset, num_bytes_to_write );

	TEMPER_CHECK_TRUE( written == true );

	// read that data back and check that it was actually written
	{
		char* data_read_from_file = cast( char* ) malloc( ( num_bytes_to_write + 1 ) * sizeof( char ) );

		bool8 read = file_read( file, offset, num_bytes_to_write, data_read_from_file );
		data_read_from_file[num_bytes_to_write] = 0;

		TEMPER_CHECK_TRUE( read );

		TEMPER_CHECK_TRUE( memcmp( data_read_from_file, data_to_write, num_bytes_to_write * sizeof( char ) ) == 0 );
		TEMPER_CHECK_TRUE( string_equals( data_read_from_file, data_to_write ) );

		free( data_read_from_file );
		data_read_from_file = NULL;
	}
}

TEMPER_PARAMETRIC( test_file_read_entire, TEMPER_FLAG_SHOULD_RUN, const char* filename, const char* expected_file_data ) {
	TEMPER_CHECK_TRUE( filename );
	TEMPER_CHECK_TRUE( expected_file_data );

	u64 expected_file_data_length = strlen( expected_file_data );

	TEMPER_CHECK_TRUE( expected_file_data_length );

	char* actual_file_data = NULL;

	// test reading file
	{
		u64 bytes_read = 0;
		bool8 read = file_read_entire( filename, &actual_file_data, &bytes_read );

		TEMPER_CHECK_TRUE( read == true );
		TEMPER_CHECK_TRUE( actual_file_data != NULL );
		TEMPER_CHECK_TRUE( bytes_read == expected_file_data_length );

		TEMPER_CHECK_TRUE( string_equals( expected_file_data, actual_file_data ) );
	}

	// test freeing file buffer
	{
		file_free_buffer( &actual_file_data );

		TEMPER_CHECK_TRUE( !actual_file_data );
	}
}

TEMPER_PARAMETRIC( test_file_get_size, TEMPER_FLAG_SHOULD_RUN, const char* filename, const u64 expected_size ) {
	TEMPER_CHECK_TRUE( filename );
	TEMPER_CHECK_TRUE( expected_size );

	File file = file_open( filename );

	TEMPER_CHECK_TRUE( file.ptr );

	u64 file_size = file_get_size( file );

	TEMPER_CHECK_TRUE( file_size );
	TEMPER_CHECK_TRUE( file_size == expected_size );

	file_close( &file );

	TEMPER_CHECK_TRUE( !file.ptr );
}

TEMPER_PARAMETRIC( test_file_rename, TEMPER_FLAG_SHOULD_RUN, const char* filename_old, const char* filename_new ) {
	TEMPER_CHECK_TRUE( filename_old );
	TEMPER_CHECK_TRUE( filename_new );

	// read old file contents
	// we will use this after renaming to test file contents is the same between the two
	char* old_file_buffer = NULL;
	u64 old_bytes_to_read = file_read_entire( filename_old, &old_file_buffer );

	TEMPER_CHECK_TRUE( old_bytes_to_read != 0 );

	File old_file = file_open( filename_old );

	TEMPER_CHECK_TRUE( old_file.ptr );

	u64 old_file_size = file_get_size( old_file );

	TEMPER_CHECK_TRUE( old_file_size );

	file_close( &old_file );

	// check that the function returned what we expect
	{
		bool8 renamed = file_rename( filename_old, filename_new );

		TEMPER_CHECK_TRUE( renamed );
	}

	// check the file infos match
	{
		File new_file = file_open( filename_new );

		TEMPER_CHECK_TRUE( new_file.ptr );

		// check file sizes are the same
		u64 new_file_size = file_get_size( new_file );

		TEMPER_CHECK_TRUE( new_file_size );
		TEMPER_CHECK_TRUE( new_file_size == old_file_size );

		file_close( &new_file );
	}

	// check that file contents are the same
	{
		char* new_file_buffer = NULL;
		u64 new_bytes_read = file_read_entire( filename_new, &new_file_buffer );

		TEMPER_CHECK_TRUE( new_bytes_read != 0 );

		TEMPER_CHECK_TRUE( old_bytes_to_read == new_bytes_read );

		TEMPER_CHECK_TRUE( memcmp( old_file_buffer, new_file_buffer, new_bytes_read ) == 0 );
		TEMPER_CHECK_TRUE( string_equals( old_file_buffer, new_file_buffer ) );
	}

	file_free_buffer( &old_file_buffer );
}

TEMPER_PARAMETRIC( test_file_delete, TEMPER_FLAG_SHOULD_RUN, const char* filename ) {
	TEMPER_CHECK_TRUE( filename );

	File file;

	// first check its there to delete
	file = file_open( filename );
	TEMPER_CHECK_TRUE( file.ptr );

	// delete it
	bool8 deleted = file_delete( filename );
	TEMPER_CHECK_TRUE( deleted );

	// now check it doesnt actually exist anymore
	file = file_open( filename );
	TEMPER_CHECK_TRUE( !file.ptr );
}

TEMPER_PARAMETRIC( test_folder_exists, TEMPER_FLAG_SHOULD_RUN, const char* path, const bool8 should_exist ) {
	TEMPER_CHECK_TRUE( path );

	bool8 exists = folder_exists( path );

	TEMPER_CHECK_TRUE( exists == should_exist );
}

TEMPER_PARAMETRIC( test_folder_create, TEMPER_FLAG_SHOULD_RUN, const char* path ) {
	TEMPER_CHECK_TRUE( path );

	bool8 exists = folder_exists( path );
	TEMPER_CHECK_TRUE( !exists );

	bool8 created = folder_create_if_it_doesnt_exist( path );
	TEMPER_CHECK_TRUE( created );
}

TEMPER_PARAMETRIC( test_folder_delete, TEMPER_FLAG_SHOULD_RUN, const char* path ) {
	TEMPER_CHECK_TRUE( path );

	bool8 exists = false;

	// first check its there to delete
	exists = folder_exists( path );
	TEMPER_CHECK_TRUE( exists );

	// delete it
	bool8 deleted = folder_delete( path );
	TEMPER_CHECK_TRUE( deleted );

	// now check it doesnt actually exist anymore
	exists = folder_exists( path );
	TEMPER_CHECK_TRUE( !exists );
}

#define TEST_FILENAME			"test_file.txt"
#define TEST_FILENAME_RENAMED	"test_file_renamed.txt"

#define TEST_FOLDER_NAME		"test_folder"

#define TEST_LINE_1				"this is an example line in the file\n"
#define TEST_LINE_2				"this is another line in the file\n"
#define TEST_LINE_3				"oh what fun we are having\n"

#define TEST_LINE_1_ALTERED		"this is an altered line in the file\n"

#define TEST_LINE_OVERWRITE		"this file has now been overwritten"

static File						g_test_file = {};

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_open, &g_test_file, TEST_FILENAME, false );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_open_or_create, &g_test_file, TEST_FILENAME );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_close, &g_test_file );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_open, &g_test_file, TEST_FILENAME, true );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_close, &g_test_file );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_open, &g_test_file, TEST_FILENAME, true );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write, &g_test_file, TEST_LINE_1, 0 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write, &g_test_file, TEST_LINE_2, strlen( TEST_LINE_1 ) );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write, &g_test_file, TEST_LINE_3, strlen( TEST_LINE_1 ) + strlen( TEST_LINE_2 ) );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_close, &g_test_file );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read_entire, TEST_FILENAME, TEST_LINE_1 TEST_LINE_2 TEST_LINE_3 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_get_size, TEST_FILENAME, strlen( TEST_LINE_1 TEST_LINE_2 TEST_LINE_3 ) );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write_entire, TEST_FILENAME, TEST_LINE_OVERWRITE );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read_entire, TEST_FILENAME, TEST_LINE_OVERWRITE );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_get_size, TEST_FILENAME, strlen( TEST_LINE_OVERWRITE ) );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write_entire, TEST_FILENAME, TEST_LINE_1 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read_entire, TEST_FILENAME, TEST_LINE_1 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_get_size, TEST_FILENAME, strlen( TEST_LINE_1 ) );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_open, &g_test_file, TEST_FILENAME, true );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write, &g_test_file, "altered", strlen( "this is an " ) );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_close, &g_test_file );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read_entire, TEST_FILENAME, TEST_LINE_1_ALTERED );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_rename, TEST_FILENAME, TEST_FILENAME_RENAMED );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_rename, TEST_FILENAME_RENAMED, TEST_FILENAME );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_delete, TEST_FILENAME );

TEMPER_INVOKE_PARAMETRIC_TEST( test_folder_exists, TEST_FOLDER_NAME, false );

TEMPER_INVOKE_PARAMETRIC_TEST( test_folder_create, TEST_FOLDER_NAME );
TEMPER_INVOKE_PARAMETRIC_TEST( test_folder_exists, TEST_FOLDER_NAME, true );
TEMPER_INVOKE_PARAMETRIC_TEST( test_folder_delete, TEST_FOLDER_NAME );


/*
================================================================================================

	Timer

================================================================================================
*/

TEMPER_TEST( test_timer_seconds, TEMPER_FLAG_SHOULD_SKIP ) {
	float64 start = time_seconds();

#ifdef _WIN32
	Sleep( 1000 );
#endif

	float64 end = time_seconds();

	TEMPER_CHECK_TRUE( doubleeq( end - start, 1.0 ) );
}

TEMPER_TEST( test_timer_milliseconds, TEMPER_FLAG_SHOULD_SKIP ) {
	float64 start = time_ms();

#ifdef _WIN32
	Sleep( 1000 );
#endif

	float64 end = time_ms();

	TEMPER_CHECK_TRUE( doubleeq( end - start, 1000.0 ) );
}

TEMPER_TEST( test_timer_microseconds, TEMPER_FLAG_SHOULD_SKIP ) {
	float64 start = time_us();

#ifdef _WIN32
	Sleep( 1000 );
#endif

	float64 end = time_us();

	TEMPER_CHECK_TRUE( doubleeq( end - start, 1000000.0 ) );
}

TEMPER_TEST( test_timer_nanoseconds, TEMPER_FLAG_SHOULD_SKIP ) {
	float64 start = time_ns();

#ifdef _WIN32
	Sleep( 1000 );
#endif

	float64 end = time_ns();

	TEMPER_CHECK_TRUE( doubleeq( end - start, 1000000000.0 ) );
}


/*
================================================================================================

	String Builder

================================================================================================
*/

TEMPER_TEST( string_builder, TEMPER_FLAG_SHOULD_RUN ) {
	StringBuilder builder = {};

	string_builder_reset( &builder );

	TEMPER_CHECK_TRUE( builder.head == NULL );
	TEMPER_CHECK_TRUE( builder.tail == NULL );

	defer( string_builder_destroy( &builder ) );

	string_builder_appendf( &builder, "this " );
	string_builder_appendf( &builder, "is " );
	string_builder_appendf( &builder, "only " );
	string_builder_appendf( &builder, "a " );
	string_builder_appendf( &builder, "test" );

	const char* actualString = string_builder_to_string( &builder );

	TEMPER_CHECK_TRUE( string_equals( actualString, "this is only a test" ) );
}


/*
================================================================================================

	Random

================================================================================================
*/

TEMPER_PARAMETRIC( random_float_within_range, TEMPER_FLAG_SHOULD_RUN, const float32 low, const float32 high ) {
	For ( u64, i, 0, 1000000 ) {
		float32 random_float = random_float32( low, high );

		TEMPER_CHECK_TRUE( random_float >= low );
		TEMPER_CHECK_TRUE( random_float <= high );
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 0.0f, 1.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 1.0f, 2.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 1.0f, 3.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 2.0f, 3.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 1.0f, 10.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 100.0f, 150.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 100.0f, 250.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 1000.0f, 2000.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 0.0f, FLT_MAX );


/*
================================================================================================

	Library

================================================================================================
*/

#include "test_dll.h"

typedef const char* ( *GetDLLNameFunc )( void );

static Library g_test_dll;

TEMPER_TEST( test_library_load, TEMPER_FLAG_SHOULD_RUN ) {
	g_test_dll = library_load( "test_dll.dll" );
	TEMPER_CHECK_TRUE( g_test_dll.ptr != NULL );
}

TEMPER_TEST( test_library_get_proc_address, TEMPER_FLAG_SHOULD_RUN ) {
	GetDLLNameFunc fp_get_dll_name = cast( GetDLLNameFunc ) library_get_proc_address( g_test_dll, "get_dll_name" );
	TEMPER_CHECK_TRUE( fp_get_dll_name != NULL );

	const char* name = fp_get_dll_name();
	TEMPER_CHECK_TRUE( string_equals( name, TEST_DLL_NAME ) );
}

TEMPER_TEST( test_library_unload, TEMPER_FLAG_SHOULD_RUN ) {
	TEMPER_CHECK_TRUE( g_test_dll.ptr != NULL );

	library_unload( &g_test_dll );

	TEMPER_CHECK_TRUE( g_test_dll.ptr == NULL );
}


//================================================================


#define TEST_PADDING "................................................................"

static void on_before_test( const temperTestInfo_t* test_info ) {
	const int pad_length_max = cast( int ) strlen( TEST_PADDING );

	const int dot_length = pad_length_max - cast( int ) strlen( test_info->testNameStr );
	assert( dot_length );

	printf( "%s %*.*s ", test_info->testNameStr, dot_length, dot_length, TEST_PADDING );
}

static void on_after_test( const temperTestInfo_t* test_info ) {
	unused( test_info );

	if ( test_info->testingFlag == TEMPER_FLAG_SHOULD_SKIP ) {
		TemperSetTextColorInternal( TEMPERDEV_COLOR_YELLOW );
		printf( "SKIPPED\n" );
		TemperSetTextColorInternal( TEMPERDEV_COLOR_DEFAULT );
	} else {
		if ( g_temperTestContext.currentTestErrorCount == 0 ) {
			TemperSetTextColorInternal( TEMPERDEV_COLOR_GREEN );
			printf( "OK" );
			TemperSetTextColorInternal( TEMPERDEV_COLOR_DEFAULT );
		} else {
			TemperSetTextColorInternal( TEMPERDEV_COLOR_RED );
			printf( "FAILED" );
			TemperSetTextColorInternal( TEMPERDEV_COLOR_DEFAULT );
		}

		printf( " (%f %s)\n", test_info->testTimeTaken, TemperGetTimeUnitStringInternal( g_temperTestContext.timeUnit ) );
	}
}

int main( int argc, char** argv ) {
	// TODO(DM): 19/1/2023: this wants to be in a test
	core_init( argc, argv, MEM_KILOBYTES( 1 ), MEM_KILOBYTES( 1 ) );
	defer( core_shutdown() );

	g_temperTestContext.callbacks.OnBeforeTest = on_before_test;
	g_temperTestContext.callbacks.OnAfterTest = on_after_test;

	TEMPER_RUN( argc, argv );

	return TEMPER_GET_EXIT_CODE();
}