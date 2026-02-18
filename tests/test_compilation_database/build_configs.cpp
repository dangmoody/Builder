// Test source file for compilation database with multiple source files.
// This validates that ALL compiled source files appear in the compilation database.

#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

static void GetBuildConfigs( BuilderOptions *options ) {
	options->generateCompilationDatabase = true;

	BuildConfig config = {
		.binaryName = "test_compilation_database_program",
		.binaryFolder = "bin",
		.languageVersion = LANGUAGE_VERSION_CPP17,
		.optimizationLevel = OPTIMIZATION_LEVEL_O0,
		.removeSymbols = false,
		.defines = { "TEST_DEFINE" },
		.sourceFiles = {
			"src/main.cpp",
			"src/helper.cpp",
		}
	};

	AddBuildConfig( options, &config );
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD
