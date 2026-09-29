#define TEMPER_IMPLEMENTATION
#include "temper.h"

#define BUILDER_IMPLEMENTATION
#include "../builder.h"

#if defined( _WIN32 )
#define TEST_DEBUG_BREAK __debugbreak
#elif defined( __linux__ )
#define TEST_DEBUG_BREAK __builtin_trap
#endif

typedef enum {
	COMPILER_CLANG	= 0,
	COMPILER_CLANGPP,
	COMPILER_GCC,
	COMPILER_GPP,
	// leave windows specific compilers last
#ifdef _WIN32
	COMPILER_CLANG_CL,
	COMPILER_MSVC,
#endif
	COMPILER_COUNT
} compiler_t;

static const char *Test_GetCompilerPath( const compiler_t compiler ) {
	switch ( compiler ) {
		case COMPILER_CLANG:	return "../tools/clang/bin/clang";
		case COMPILER_CLANGPP:	return "../tools/clang/bin/clang++";
		case COMPILER_GCC:		return "../tools/gcc/bin/gcc";
		case COMPILER_GPP:		return "../tools/gcc/bin/g++";
#ifdef _WIN32
		case COMPILER_MSVC:		return g_msvcInstall.compilerPath;
		case COMPILER_CLANG_CL:	return "../tools/clang/bin/clang-cl";
#endif
	}

	assert( false && "Bad compiler_t specified." );

	return NULL;
}

typedef struct {
	arena_t		*stringListArena;

	StringList	fileExtensionsToDelete;
	StringList	filesToExclude;

	// filled out by the callback
	StringList	deferredFilesToDelete;
	StringList	deferredFoldersToDelete;
} testCleanupContext_t;

static bool Test_DeleteFile( const char *filename ) {
#if defined( _WIN32 )
	if ( !DeleteFile( filename ) ) {
		Builder_Error( "Failed to delete file \"%s\": GetLastError(): 0x%X\n", filename, GetLastError() );
		return false;
	}
#elif defined( __linux__ )
	if ( remove( filename ) != 0 ) {
		int err = errno;
		Builder_Error( "Failed to delete file \"%s\": errno: %d, \"%s\"\n", filename, err, strerror( err ) );
		return false;
	}
#endif

	return true;
}

static bool Test_DeleteFolder( const char *folder ) {
#if defined( _WIN32 )
	if ( !RemoveDirectory( folder ) ) {
		Builder_Error( "Failed to delete folder \"%s\": GetLastError(): 0x%X\n", folder, GetLastError() );
		return false;
	}
#elif defined( __linux__ )
	if ( rmdir( folder ) != 0 ) {
		int err = errno;
		Builder_Error( "Failed to delete folder \"%s\": errno: %d, \"%s\"\n", folder, err, strerror( err ) );
		return false;
	}
#endif

	return true;
}

static void Test_OnGeneratedFilesFound( arena_t *resultsArena, fileInfo_t *fileInfo, void *data ) {
	BUILDER_ASSERT( resultsArena );
	BUILDER_ASSERT( fileInfo );
	BUILDER_ASSERT( data );

	testCleanupContext_t *context = (testCleanupContext_t *) data;

	if ( fileInfo->isDirectory ) {
		const char *fullFilename = Builder_FormatString( resultsArena, "%s", fileInfo->fullFilename );
		Builder_StringListPush( context->stringListArena, &context->deferredFoldersToDelete, fullFilename );
	} else {
		for ( builderStringChunk_t *chunk = context->filesToExclude.head; chunk; chunk = chunk->next ) {
			for ( uint32_t folderIndex = 0; folderIndex < chunk->count; folderIndex++ ) {
				const char *fileToExclude = chunk->items[folderIndex];

				if ( Builder_StringEquals( fileInfo->filename, fileToExclude ) ) {
					return;
				}
			}
		}

		bool foundFile = false;

		for ( builderStringChunk_t *chunk = context->fileExtensionsToDelete.head; chunk && !foundFile; chunk = chunk->next ) {
			for ( uint32_t folderIndex = 0; folderIndex < chunk->count; folderIndex++ ) {
				const char *fileExtensionToDelete = chunk->items[folderIndex];

				if ( Builder_PathEndsWith( fileInfo->filename, fileExtensionToDelete ) ) {
					const char *fullFilename = Builder_FormatString( resultsArena, "%s", fileInfo->fullFilename );
					Builder_StringListPush( context->stringListArena, &context->deferredFilesToDelete, fullFilename );

					foundFile = true;
					break;
				}
			}
		}
	}
}

TEMPER_TEST_PARAMETRIC( TestBuild, TEMPER_FLAG_SHOULD_RUN, const char *testFolder, const char *programFilename, const int32_t expectedBuildEXEExitCode, const int32_t expectedProgramExitCode, const bool alsoCompileCPP ) {
	arena_t testScratch = { 0 };

	const char *buildSourceFilename = Builder_FormatString( &testScratch, "%s/build.c", testFolder );
	const char *buildEXEFilename = Builder_FormatString( &testScratch, "%s/build%s", testFolder, Builder_GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );

	arenaRewindSpot_t testScratchStart = Builder_ArenaTell( &testScratch );

	for ( int32_t compilerIndex = 0; compilerIndex < COMPILER_COUNT; compilerIndex++ ) {
		compiler_t compiler = (compiler_t) compilerIndex;

		if ( !alsoCompileCPP && ( compiler == COMPILER_CLANGPP || compiler == COMPILER_GPP ) ) {
			continue;
		}

		const char *compilerName = NULL;
		switch ( compiler ) {
			case COMPILER_CLANG:	compilerName = "clang";		break;
			case COMPILER_CLANGPP:	compilerName = "clang++";	break;
			case COMPILER_GCC:		compilerName = "gcc";		break;
			case COMPILER_GPP:		compilerName = "g++";		break;
#ifdef _WIN32
			case COMPILER_MSVC:		compilerName = "msvc";		break;
			case COMPILER_CLANG_CL: compilerName = "clang-cl";	break;
#endif
		}

		TEMPER_CHECK_TRUE( compilerName );

		printf( "Building \"%s\" for compiler: %s\n", buildSourceFilename, compilerName );

		Builder_RewindArena( &testScratch, &testScratchStart );

		// initial build test
		{
			char *output = NULL;

			stringBuilder_t sb = { 0 };
			StringBuilder_Appendf( &testScratch, &sb, "%s ", Test_GetCompilerPath( COMPILER_CLANG ) );
			StringBuilder_Appendf( &testScratch, &sb, "-o " );
			StringBuilder_Appendf( &testScratch, &sb, "%s ", buildEXEFilename );
			StringBuilder_Appendf( &testScratch, &sb, "%s ", buildSourceFilename );
			char *buildCMDArgs = StringBuilder_ToString( &testScratch, &sb, NULL );

			printf( "Raw compile args: %s\n", buildCMDArgs );

			int32_t buildCMDExitCode = Builder_RunProcess( &testScratch, buildCMDArgs, false, &output );

			printf( "%s\n", output );

			TEMPER_CHECK_TRUE_QM( buildCMDExitCode == 0, "Failed to do initial build of %s via the raw compiler argument.\n", buildSourceFilename );
		}

		// run the build EXE
		{
			char *output = NULL;

			stringBuilder_t sb = { 0 };
			StringBuilder_Appendf( &testScratch, &sb, "%s ", buildEXEFilename );
			StringBuilder_Appendf( &testScratch, &sb, "--%s ", compilerName );
			const char *buildArgs = StringBuilder_ToString( &testScratch, &sb, NULL );

			printf( "Test build.exe args: %s\n", buildArgs );

			int32_t buildEXEExitCode = Builder_RunProcess( &testScratch, buildArgs, false, &output );

			printf( "%s\n", output );

			TEMPER_CHECK_TRUE_QM( buildEXEExitCode == expectedBuildEXEExitCode, "\"%s\" should've returned %d but instead returned %d.\n", buildSourceFilename, expectedBuildEXEExitCode, buildEXEExitCode );

			if ( expectedBuildEXEExitCode != 0 ) {
				printf( "Build was expected to fail, and we got the exit code we were looking for.  This is fine.\n" );
			}
		}

		// run the program we just built
		if ( programFilename ) {
			char *output = NULL;
			int32_t programExitCode = Builder_RunProcess( &testScratch, programFilename, false, &output );

			printf( "%s\n", output );

			TEMPER_CHECK_TRUE_M( programExitCode == expectedProgramExitCode, "Program \"%s\" should've returned %d but instead returned %d.\n", programFilename, expectedProgramExitCode, programExitCode );

			if ( expectedProgramExitCode != 0 ) {
				printf( "Program was expected to fail, and we got the exit code we were looking for.  This is fine.\n" );
			}
		}

		// delete all generated files and folders
		// leave this last
		{
			testCleanupContext_t context = {
				.stringListArena = &testScratch,
				.fileExtensionsToDelete = MakeStringList(
					".builder-dependencies",
					".exe",
					".dll",
					".lib",
					".pdb",
					".exp",
					".ilk",
					".so",
					".a",
					".o",
					// wayland generated files are just auto-generated C source and header files
					".c",
					".h"
				),
				.filesToExclude = MakeStringList(
					"build.exe",
					"build",
				),
			};

			// if this test contains any of the folders that we care about:
			// 	delete every file inside it
			// 	add it a list of folders to delete later
			{
				StringList foldersToDelete = MakeStringList(
					"bin",
					"intermediate",
					"visual_studio",
				);

				for ( builderStringChunk_t *chunk = foldersToDelete.head; chunk; chunk = chunk->next ) {
					for ( uint32_t fileIndex = 0; fileIndex < chunk->count; fileIndex++ ) {
						const char *folderToCheck = Builder_FormatString( &testScratch, "%s%c%s", testFolder, BUILDER_PATH_SEPARATOR, chunk->items[fileIndex] );

						if ( Builder_FolderExists( folderToCheck ) ) {
							bool visited = Builder_VisitFiles( &testScratch, folderToCheck, BUILDER_FILE_VISIT_FILES | BUILDER_FILE_VISIT_FOLDERS | BUILDER_FILE_VISIT_RECURSIVE, Test_OnGeneratedFilesFound, &context );
							TEMPER_CHECK_TRUE( visited );
						}
					}
				}
			}

			for ( builderStringChunk_t *chunk = context.deferredFilesToDelete.head; chunk; chunk = chunk->next ) {
				for ( uint32_t fileIndex = 0; fileIndex < chunk->count; fileIndex++ ) {
					const char *filename = chunk->items[fileIndex];

					bool deleted = Test_DeleteFile( filename );

					TEMPER_CHECK_TRUE_M( deleted, "Failed to delete file \"%s\".  The tests should properly clean up after themselves.\n", filename );
				}
			}

			for ( builderStringChunk_t *chunk = context.deferredFoldersToDelete.head; chunk; chunk = chunk->next ) {
				for ( uint32_t folderIndex = 0; folderIndex < chunk->count; folderIndex++ ) {
					const char *folder = chunk->items[folderIndex];

					bool deleted = Test_DeleteFolder( folder );

					TEMPER_CHECK_TRUE_M( deleted, "Failed to delete folder \"%s\".  The tests should properly clean up after themselves.\n", folder );
				}
			}
		}

		printf( "\n" );
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "single_file",             "single_file/test_build_single_file",        0, 0, true  );
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "multiple_files",           "multiple_files/test_build_multiple_files", 0, 0, true  );
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "static_lib",               "static_lib/test_static_lib_program",       0, 5, true  );
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "dynamic_lib",              "dynamic_lib/test_dynamic_lib_program",     0, 5, true  );
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "only_self_rebuild_config", NULL,                                       1, 0, true  );
// the SDL test basically tests everything builder can do, more or less
// so leave it last
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "sdl3",                     "sdl3/bin/sdl-demo-app",                    0, 0, false );

// generates compile_commands.json for each compiler and checks clangd and clang-tidy can both use it
// both of those tools exit non-zero if they can't load the database or the source files fail to compile with it
TEMPER_TEST( TestCompilationDatabase, TEMPER_FLAG_SHOULD_RUN ) {
	arena_t testScratch = { 0 };

	const char *testFolder = "compilation_database";
	const char *clangdPath = "../tools/clang/bin/clangd";
	const char *clangTidyPath = "../tools/clang/bin/clang-tidy";

	const char *buildSourceFilename = Builder_FormatString( &testScratch, "%s/build.c", testFolder );
	const char *buildEXEFilename = Builder_FormatString( &testScratch, "%s/build%s", testFolder, Builder_GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );
	const char *compilationDatabaseFilename = Builder_FormatString( &testScratch, "%s/compile_commands.json", testFolder );

	StringList sourceFiles = MakeStringList(
		Builder_FormatString( &testScratch, "%s/src/main.c", testFolder ),
		Builder_FormatString( &testScratch, "%s/src/helper.c", testFolder ),
	);

	// build the build EXE
	// running it with --compile-commands never calls Build() so it never rebuilds itself, once is enough
	{
		printf( "Running %s...\n", buildEXEFilename );

		char *output = NULL;
		int32_t buildCMDExitCode = Builder_RunProcess( &testScratch, buildEXEFilename, false, &output );

		printf( "%s\n", output );

		TEMPER_CHECK_TRUE_QM( buildCMDExitCode == 0, "Failed to run the build executable.\n" );
	}

	arenaRewindSpot_t testScratchStart = Builder_ArenaTell( &testScratch );

	for ( int32_t compilerIndex = 0; compilerIndex < COMPILER_COUNT; compilerIndex++ ) {
		compiler_t compiler = (compiler_t) compilerIndex;

		const char *compilerName = NULL;
		switch ( compiler ) {
			case COMPILER_CLANG:	compilerName = "clang";		break;
			case COMPILER_CLANGPP:	compilerName = "clang++";	break;
			case COMPILER_GCC:		compilerName = "gcc";		break;
			case COMPILER_GPP:		compilerName = "g++";		break;
#ifdef _WIN32
			case COMPILER_MSVC:		compilerName = "msvc";		break;
			case COMPILER_CLANG_CL: compilerName = "clang-cl";	break;
#endif
		}

		TEMPER_CHECK_TRUE( compilerName );

		printf( "Generating compilation database for \"%s\" for compiler: %s\n", buildSourceFilename, compilerName );

		Builder_RewindArena( &testScratch, &testScratchStart );

		// generate compile_commands.json
		{
			const char *buildArgs = Builder_FormatString( &testScratch, "%s --%s --compile-commands", buildEXEFilename, compilerName );

			printf( "Test build.exe args: %s\n", buildArgs );

			char *output = NULL;
			int32_t buildEXEExitCode = Builder_RunProcess( &testScratch, buildArgs, false, &output );

			printf( "%s\n", output );

			TEMPER_CHECK_TRUE_QM( buildEXEExitCode == 0, "\"%s\" should've returned 0 but instead returned %d.\n", buildArgs, buildEXEExitCode );
		}

		// clangd
		for ( builderStringChunk_t *chunk = sourceFiles.head; chunk; chunk = chunk->next ) {
			for ( uint32_t sourceFileIndex = 0; sourceFileIndex < chunk->count; sourceFileIndex++ ) {
				const char *clangdArgs = Builder_FormatString( &testScratch, "%s --check=%s", clangdPath, chunk->items[sourceFileIndex] );

				printf( "clangd args: %s\n", clangdArgs );

				char *output = NULL;
				int32_t clangdExitCode = Builder_RunProcess( &testScratch, clangdArgs, false, &output );

				printf( "%s\n", output );

				TEMPER_CHECK_TRUE_M( clangdExitCode == 0, "\"%s\" should've returned 0 but instead returned %d.\n", clangdArgs, clangdExitCode );
			}
		}

		// clang-tidy
		{
			stringBuilder_t sb = { 0 };
			StringBuilder_Appendf( &testScratch, &sb, "%s -p %s ", clangTidyPath, testFolder );

			for ( builderStringChunk_t *chunk = sourceFiles.head; chunk; chunk = chunk->next ) {
				for ( uint32_t sourceFileIndex = 0; sourceFileIndex < chunk->count; sourceFileIndex++ ) {
					StringBuilder_Appendf( &testScratch, &sb, "%s ", chunk->items[sourceFileIndex] );
				}
			}

			const char *clangTidyArgs = StringBuilder_ToString( &testScratch, &sb, NULL );

			printf( "clang-tidy args: %s\n", clangTidyArgs );

			char *output = NULL;
			int32_t clangTidyExitCode = Builder_RunProcess( &testScratch, clangTidyArgs, false, &output );

			printf( "%s\n", output );

			TEMPER_CHECK_TRUE_M( clangTidyExitCode == 0, "\"%s\" should've returned 0 but instead returned %d.\n", clangTidyArgs, clangTidyExitCode );
		}

		// make the test clean up after itself
		{
			bool deleted = Test_DeleteFile( compilationDatabaseFilename );

			TEMPER_CHECK_TRUE_M( deleted, "Failed to delete file \"%s\".  The tests should properly clean up after themselves.\n", compilationDatabaseFilename );
		}

		printf( "\n" );
	}
}

int main( int argc, char **argv ) {
	arena_t arena = { 0 };

#ifdef _WIN32
	Builder_GetMSVCInstall( &arena, &g_msvcInstall );
#endif

	TEMPER_RUN( argc, argv );

	int exitCode = TEMPER_GET_EXIT_CODE();

	if ( exitCode != 0 ) {
		TEST_DEBUG_BREAK();
	}

	return exitCode;
}
