#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig library = {
		.name				= "library",
		.binary_folder		= "bin",
#if defined( _WIN32 )
		.binary_name		= "test_dynamic_lib",
#elif defined( __linux__ )
		.binary_name		= "libtest_dynamic_lib",
#endif
		.binary_type		= BINARY_TYPE_DYNAMIC_LIBRARY,
		.source_files		= { "lib/*.cpp" },
		.defines			= { "DYNAMIC_LIBRARY_EXPORTS" },
#ifdef __linux__
		.ignore_warnings	= { "-fPIC" },
#endif
	};

	BuildConfig program = {
		.depends_on				= { library },
		.name					= "program",
		.binary_folder			= "bin",
		.binary_name			= "test_dynamic_library_program",
		.source_files			= { "program/*.cpp" },
		.additional_includes	= { "lib" },
		.additional_lib_paths	= { "bin" },
		// TODO(DM): 07/10/2025: does this mean we want build scripts to ignore file extensions?
#if defined( _WIN32 )
		.additional_libs		= { "test_dynamic_lib.lib" },
#elif defined( __linux__ )
		.additional_libs		= { "libtest_dynamic_lib" },
		.ignore_warnings		= { "-fPIC" },
#endif
	};

	// only need to add program config
	// program depends on library, so library will get added automatically when adding program
	add_build_config( options, &program );
}