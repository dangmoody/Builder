// Test source file for compilation database with multiple source files.
// This validates that ALL compiled source files appear in the compilation database.

#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

static void get_build_configs( BuilderOptions* options ) {

    options->generate_compilation_database = true;
    
    BuildConfig config = {
        .binary_name = "test_compilation_database_program",
        .binary_folder = "bin",
        .language_version = LANGUAGE_VERSION_CPP17,
        .optimization_level = OPTIMIZATION_LEVEL_O0,
        .remove_symbols = false,
        .defines = { "TEST_DEFINE" },
        .source_files = {
            "src/main.cpp",
            "src/helper.cpp",
        }
    };

    add_build_config( options, &config );
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD
