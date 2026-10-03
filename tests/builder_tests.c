#define BUILDER_IMPLEMENTATION
#include "../builder.h"

#define TEMPER_IMPLEMENTATION
#include "temper.h"

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

typedef struct {
	const char	*objectFilePrefix;

	// filled out by the callback
	const char	*objectFilename;
	uint32_t	matchCount;
} testFindObjectContext_t;

static void Test_OnIntermediateFileFound( arena_t *resultsArena, fileInfo_t *fileInfo, void *data ) {
	BUILDER_ASSERT( resultsArena );
	BUILDER_ASSERT( fileInfo );
	BUILDER_ASSERT( data );

	testFindObjectContext_t *context = (testFindObjectContext_t *) data;

	if ( fileInfo->isDirectory ) {
		return;
	}

	if ( !Builder_StringStartsWith( fileInfo->filename, context->objectFilePrefix ) ) {
		return;
	}

	if ( !Builder_PathEndsWith( fileInfo->filename, ".o" ) ) {
		return;
	}

	context->objectFilename = Builder_FormatString( resultsArena, "%s", fileInfo->fullFilename );
	context->matchCount++;
}

static char *Test_RunProcess( arena_t *arena, const char *args, const int32_t expectedExitCode, const bool quitOnFail ) {
	printf( "Running: %s\n", args );

	char *output = NULL;
	int32_t exitCode = Builder_RunProcess( arena, args, false, &output );

	printf( "%s\n", output );

	if ( quitOnFail ) {
		TEMPER_CHECK_TRUE_QM( exitCode == expectedExitCode, "\"%s\" should've returned %d but instead returned %d.\n", args, expectedExitCode, exitCode );
	} else {
		TEMPER_CHECK_TRUE_M( exitCode == expectedExitCode, "\"%s\" should've returned %d but instead returned %d.\n", args, expectedExitCode, exitCode );
	}

	return output;
}

typedef struct {
	const char	*filename;
	uint8_t		*originalContents;
	uint64_t	originalSize;
} testFileEdit_t;

static testFileEdit_t Test_AppendToFile( arena_t *arena, const char *filename, const char *text ) {
	testFileEdit_t edit = { .filename = filename };

	edit.originalContents = Builder_ReadEntireFile( arena, filename, &edit.originalSize );

	TEMPER_CHECK_TRUE_QM( edit.originalContents, "Failed to read \"%s\".\n", filename );

	const char *editedContents = Builder_FormatString( arena, "%.*s\n%s\n", edit.originalSize, (const char *) edit.originalContents, text );

	bool written = Builder_WriteEntireFile( filename, (const uint8_t *) editedContents, strlen( editedContents ) );

	TEMPER_CHECK_TRUE_QM( written, "Failed to edit \"%s\".\n", filename );

	return edit;
}

static void Test_UndoFileEdit( const testFileEdit_t *edit ) {
	bool written = Builder_WriteEntireFile( edit->filename, edit->originalContents, edit->originalSize );

	TEMPER_CHECK_TRUE_QM( written, "Failed to undo the edit to \"%s\".  It's been left modified, restore it by hand.\n", edit->filename );
}

// every config that compiles anything prints "Compiling <count> files ..."
// so add them all up to get how many files got compiled across the whole build
static uint32_t Test_GetCompiledFileCount( const char *buildOutput ) {
	const char *compilingPrefix = "Compiling ";

	uint32_t totalCount = 0;

	for ( const char *found = strstr( buildOutput, compilingPrefix ); found; found = strstr( found + 1, compilingPrefix ) ) {
		uint32_t count = 0;
		if ( sscanf( found + strlen( compilingPrefix ), "%u files", &count ) != 1 ) {
			continue;
		}

		totalCount += count;
	}

	return totalCount;
}

static void Test_CheckIncrementalRebuild( arena_t *arena, const char *buildArgs, const char *fileToEdit, const uint32_t expectedCompiledFileCount ) {
	testFileEdit_t edit = Test_AppendToFile( arena, fileToEdit, "// builder test edit" );

	// dont quit on failure, the edit still needs undoing
	char *output = Test_RunProcess( arena, buildArgs, 0, false );

	uint32_t compiledFileCount = Test_GetCompiledFileCount( output );

	TEMPER_CHECK_TRUE_M( compiledFileCount == expectedCompiledFileCount, "Editing \"%s\" should've rebuilt %u files, but %u files were compiled.\n", fileToEdit, expectedCompiledFileCount, compiledFileCount );

	Test_UndoFileEdit( &edit );

	output = Test_RunProcess( arena, buildArgs, 0, true );

	compiledFileCount = Test_GetCompiledFileCount( output );

	TEMPER_CHECK_TRUE_M( compiledFileCount == expectedCompiledFileCount, "Undoing the edit to \"%s\" should've rebuilt %u files, but %u files were compiled.\n", fileToEdit, expectedCompiledFileCount, compiledFileCount );
}

TEMPER_TEST_PARAMETRIC( TestBuild, TEMPER_FLAG_SHOULD_RUN,
	const char *testFolder,
	const char *programFilename,
	const int32_t expectedBuildEXEExitCode,
	const int32_t expectedProgramExitCode,
	const bool alsoCompileCPP,
	const char *sourceFileToEdit,
	const char *headerFileToEdit,
	const uint32_t headerDependentCount,
	const char *newSourceFile )
{
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
			const char *buildCMDArgs = Builder_FormatString( &testScratch, "%s -o %s %s", Test_GetCompilerPath( COMPILER_CLANG ), buildEXEFilename, buildSourceFilename );

			Test_RunProcess( &testScratch, buildCMDArgs, 0, true );
		}

		const char *buildArgs = Builder_FormatString( &testScratch, "%s --%s", buildEXEFilename, compilerName );

		// run the build EXE
		{
			Test_RunProcess( &testScratch, buildArgs, expectedBuildEXEExitCode, true );

			if ( expectedBuildEXEExitCode != 0 ) {
				printf( "Build was expected to fail, and we got the exit code we were looking for.  This is fine.\n" );
			}
		}

		// run the program we just built
		if ( programFilename ) {
			Test_RunProcess( &testScratch, programFilename, expectedProgramExitCode, false );

			if ( expectedProgramExitCode != 0 ) {
				printf( "Program was expected to fail, and we got the exit code we were looking for.  This is fine.\n" );
			}
		}

		// make a partial edit to the source file and rebuild
		// only the source file(s) we edited should get rebuilt
		// afterwards undo the change and build again so its like we never touched it
		if ( sourceFileToEdit ) {
			Test_CheckIncrementalRebuild( &testScratch, buildArgs, sourceFileToEdit, 1 );
		}

		// change the local header file that one of our source files depends on
		// only the source file(s) that rely on that header should get rebuilt
		// afterwards undo the change and build again so its like we never touched it
		if ( headerFileToEdit ) {
			Test_CheckIncrementalRebuild( &testScratch, buildArgs, headerFileToEdit, headerDependentCount );
		}

		// delete one of the intermediate files
		// only the source file that intermediate file is built from should get rebuilt
		if ( sourceFileToEdit ) {
			// object files are named after the source file without its folder or extension
			const char *sourceFilename = NULL;
			if ( !sourceFilename ) sourceFilename = strrchr( sourceFileToEdit, '/' );
			if ( !sourceFilename ) sourceFilename = strrchr( sourceFileToEdit, '\\' );
			if ( !sourceFilename ) {
				sourceFilename = sourceFileToEdit;
			} else {
				sourceFilename++;
			}

			const char *extension = strrchr( sourceFilename, '.' );
			uint64_t sourceFilenameLength = extension ? (uint64_t) ( extension - sourceFilename ) : strlen( sourceFilename );

			testFindObjectContext_t context = {
				.objectFilePrefix = Builder_FormatString( &testScratch, "%.*s_", sourceFilenameLength, sourceFilename ),
			};

			const char *intermediateFolder = Builder_FormatString( &testScratch, "%s%cintermediate", testFolder, BUILDER_PATH_SEPARATOR );

			bool visited = Builder_VisitFiles( &testScratch, intermediateFolder, BUILDER_FILE_VISIT_FILES, Test_OnIntermediateFileFound, &context );

			TEMPER_CHECK_TRUE_QM( visited, "Failed to look through \"%s\".\n", intermediateFolder );
			TEMPER_CHECK_TRUE_QM( context.matchCount == 1, "Expected to find exactly 1 object file for \"%s\" in \"%s\", but found %u.\n", sourceFileToEdit, intermediateFolder, context.matchCount );

			bool deleted = Test_DeleteFile( context.objectFilename );

			TEMPER_CHECK_TRUE_QM( deleted, "Failed to delete \"%s\".\n", context.objectFilename );

			char *output = Test_RunProcess( &testScratch, buildArgs, 0, true );

			uint32_t compiledFileCount = Test_GetCompiledFileCount( output );

			TEMPER_CHECK_TRUE_M( compiledFileCount == 1, "Only \"%s\" should've been rebuilt after deleting \"%s\", but %u files were compiled.\n", sourceFileToEdit, context.objectFilename, compiledFileCount );
		}

		// dont change anything but build again
		// the build should be totally skipped, nothing should happen
		// builder only links when something compiled or the binary is missing, so nothing compiling means nothing happened
		if ( expectedBuildEXEExitCode == 0 ) {
			char *output = Test_RunProcess( &testScratch, buildArgs, 0, false );

			uint32_t compiledFileCount = Test_GetCompiledFileCount( output );

			TEMPER_CHECK_TRUE_M( compiledFileCount == 0, "Nothing changed so the build should've been skipped, but %u files were compiled.\n", compiledFileCount );
		}

		// make a change to the code that will cause a compilation error
		// the build should fail
		// afterwards undo the change and build again so its like we never touched it
		if ( sourceFileToEdit ) {
			testFileEdit_t edit = Test_AppendToFile( &testScratch, sourceFileToEdit, "this wont compile" );

			// dont quit on failure, the edit still needs undoing
			Test_RunProcess( &testScratch, buildArgs, 1, false );

			Test_UndoFileEdit( &edit );

			char *output = Test_RunProcess( &testScratch, buildArgs, 0, true );

			uint32_t compiledFileCount = Test_GetCompiledFileCount( output );

			TEMPER_CHECK_TRUE_M( compiledFileCount == 1, "Only \"%s\" should've been rebuilt after fixing its compile error, but %u files were compiled.\n", sourceFileToEdit, compiledFileCount );
		}

		// make a change to the code that will cause a link error
		// the build should fail
		// afterwards undo the change and build again so its like we never touched it
		if ( sourceFileToEdit ) {
			const char *linkErrorCode =
				"void Test_UndefinedFunction( void );\n"
				"void Test_CauseLinkError( void );\n"
				"void Test_CauseLinkError( void ) { Test_UndefinedFunction(); }";

			testFileEdit_t edit = Test_AppendToFile( &testScratch, sourceFileToEdit, linkErrorCode );

			// dont quit on failure, the edit still needs undoing
			char *output = Test_RunProcess( &testScratch, buildArgs, 1, false );

			// the edited file still has to compile
			uint32_t compiledFileCount = Test_GetCompiledFileCount( output );

			TEMPER_CHECK_TRUE_M( compiledFileCount == 1, "Only \"%s\" should've been compiled before the link failed, but %u files were compiled.\n", sourceFileToEdit, compiledFileCount );

			Test_UndoFileEdit( &edit );

			output = Test_RunProcess( &testScratch, buildArgs, 0, true );

			compiledFileCount = Test_GetCompiledFileCount( output );

			TEMPER_CHECK_TRUE_M( compiledFileCount == 1, "Only \"%s\" should've been rebuilt after fixing its link error, but %u files were compiled.\n", sourceFileToEdit, compiledFileCount );
		}

		// add a file then build again
		// only the new file should be compiled
		if ( newSourceFile ) {
			// valid C and C++
			// the prototype is there so -Wmissing-prototypes doesnt complain
			const char *newSourceFileContents =
				"void Test_NewFileFunction( void );\n"
				"void Test_NewFileFunction( void ) {}\n";

			bool written = Builder_WriteEntireFile( newSourceFile, (const uint8_t *) newSourceFileContents, strlen( newSourceFileContents ) );

			TEMPER_CHECK_TRUE_QM( written, "Failed to create \"%s\".\n", newSourceFile );

			// dont quit on failure, the new file still needs deleting
			char *output = Test_RunProcess( &testScratch, buildArgs, 0, false );

			uint32_t compiledFileCount = Test_GetCompiledFileCount( output );

			TEMPER_CHECK_TRUE_M( compiledFileCount == 1, "Only \"%s\" should've been compiled after adding it, but %u files were compiled.\n", newSourceFile, compiledFileCount );
		}

		// remove the file we just created then build again
		// the build should succeed
		if ( newSourceFile ) {
			bool deleted = Test_DeleteFile( newSourceFile );

			TEMPER_CHECK_TRUE_M( deleted, "Failed to delete \"%s\".  It's been left behind, delete it by hand.\n", newSourceFile );

			Test_RunProcess( &testScratch, buildArgs, 0, true );
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

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "single_file",             "single_file/test_build_single_file",        0, 0, true,  "single_file/main.c",         NULL,                        0, NULL                                         );
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "multiple_files",           "multiple_files/test_build_multiple_files", 0, 0, true,  "multiple_files/src/test1.c", "multiple_files/src/test.h", 3, "multiple_files/src/builder_test_new_file.c" );
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "static_lib",               "static_lib/test_static_lib_program",       0, 5, true,  "static_lib/program/main.c",  "static_lib/lib/mathlib.h",  2, NULL                                         );
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "dynamic_lib",              "dynamic_lib/test_dynamic_lib_program",     0, 5, true,  "dynamic_lib/program/main.c", "dynamic_lib/lib/mathlib.h", 2, NULL                                         );
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "only_self_rebuild_config", NULL,                                       1, 0, true,  NULL,                         NULL,                        0, NULL                                         );
// the SDL test basically tests everything builder can do, more or less
// so leave it last
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "sdl3",                     "sdl3/bin/sdl-demo-app",                    0, 0, false, "sdl3/demo-app/demo-app.cpp", "sdl3/src/audio/SDL_wave.h", 1, "sdl3/demo-app/builder_test_new_file.cpp"    );

TEMPER_TEST( TestVisualStudio, TEMPER_FLAG_SHOULD_RUN ) {
	// TODO: DM: 29/09/2026: this
}

TEMPER_TEST( TestVSCodeJSON, TEMPER_FLAG_SHOULD_RUN ) {
	arena_t testScratch = { 0 };

	const char *testFolder = "vs_code_json";

	const char *buildEXEFilename = Builder_FormatString( &testScratch, "%s/build%s", testFolder, Builder_GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );
	const char *dotVSCodeFolder = Builder_FormatString( &testScratch, "%s/.vscode", testFolder );

	typedef struct {
		const char	*filename;
		StringList	expectedEntries;
	} expectedJSONFile_t;

	expectedJSONFile_t expectedFiles[] = {
		{
			.filename = Builder_FormatString( &testScratch, "%s/c_cpp_properties.json", dotVSCodeFolder ),
			.expectedEntries = MakeStringList(
				"\"name\": \"config\"",
				"\"intelliSenseMode\": \"linux-clang-x64\"",
				"\"version\": 4",
			),
		},
		{
			.filename = Builder_FormatString( &testScratch, "%s/tasks.json", dotVSCodeFolder ),
			.expectedEntries = MakeStringList(
				"\"label\": \"Build config\"",
				Builder_FormatString( &testScratch, "\"command\": \"%s\"", buildEXEFilename ),
				"\"" ARG_CONFIG "config\"",
				"\"--release\"",
			),
		},
		{
			.filename = Builder_FormatString( &testScratch, "%s/launch.json", dotVSCodeFolder ),
			.expectedEntries = MakeStringList(
				"\"program\": \"bin/debug/test_generate_vs_code_json\"",
				"\"program\": \"bin/release/test_generate_vs_code_json\"",
				"\"type\": \"cppdbg\"",
				"\"MIMode\": \"gdb\"",
				"\"cwd\": \"${workspaceFolder}\"",
			),
		},
	};

	// build the build EXE
	Test_RunProcess( &testScratch, buildEXEFilename, 0, true );

	// generate the json files
	{
		const char *buildArgs = Builder_FormatString( &testScratch, "%s --vscode", buildEXEFilename );

		Test_RunProcess( &testScratch, buildArgs, 0, true );
	}

	// check each file has everything we asked for
	for ( uint32_t fileIndex = 0; fileIndex < BUILDER_COUNT_OF( expectedFiles ); fileIndex++ ) {
		expectedJSONFile_t *expectedFile = &expectedFiles[fileIndex];

		uint64_t fileSize = 0;
		uint8_t *fileData = Builder_ReadEntireFile( &testScratch, expectedFile->filename, &fileSize );

		TEMPER_CHECK_TRUE_M( fileData, "Failed to read \"%s\".  It should've been generated.\n", expectedFile->filename );

		if ( !fileData ) {
			continue;
		}

		// file data isnt null terminated
		const char *fileContents = Builder_FormatString( &testScratch, "%.*s", fileSize, (const char *) fileData );

		for ( builderStringChunk_t *chunk = expectedFile->expectedEntries.head; chunk; chunk = chunk->next ) {
			for ( uint32_t entryIndex = 0; entryIndex < chunk->count; entryIndex++ ) {
				const char *expectedEntry = chunk->items[entryIndex];

				TEMPER_CHECK_TRUE_M( Builder_StringContains( fileContents, expectedEntry ), "\"%s\" is missing expected entry: %s\n", expectedFile->filename, expectedEntry );
			}
		}
	}

	// make the test clean up after itself
	{
		for ( uint32_t fileIndex = 0; fileIndex < BUILDER_COUNT_OF( expectedFiles ); fileIndex++ ) {
			const char *filename = expectedFiles[fileIndex].filename;

			bool deleted = Test_DeleteFile( filename );

			TEMPER_CHECK_TRUE_M( deleted, "Failed to delete file \"%s\".  The tests should properly clean up after themselves.\n", filename );
		}

		bool deleted = Test_DeleteFolder( dotVSCodeFolder );

		TEMPER_CHECK_TRUE_M( deleted, "Failed to delete folder \"%s\".  The tests should properly clean up after themselves.\n", dotVSCodeFolder );
	}
}

TEMPER_TEST( TestZedJSON, TEMPER_FLAG_SHOULD_RUN ) {
	arena_t testScratch = { 0 };

	const char *testFolder = "zed_json";

	const char *buildEXEFilename = Builder_FormatString( &testScratch, "%s/build%s", testFolder, Builder_GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );
	const char *dotZedFolder = Builder_FormatString( &testScratch, "%s/.zed", testFolder );

	typedef struct {
		const char	*filename;
		StringList	expectedEntries;
	} expectedJSONFile_t;

	expectedJSONFile_t expectedFiles[] = {
		{
			.filename = Builder_FormatString( &testScratch, "%s/tasks.json", dotZedFolder ),
			.expectedEntries = MakeStringList(
				"\"label\": \"Build config\"",
				Builder_FormatString( &testScratch, "\"command\": \"%s\"", buildEXEFilename ),
				"\"" ARG_CONFIG "config\"",
				"\"--release\"",
			),
		},
		{
			.filename = Builder_FormatString( &testScratch, "%s/debug.json", dotZedFolder ),
			.expectedEntries = MakeStringList(
				"\"label\": \"Debug test_generate_zed_json (debug)\"",
				"\"label\": \"Debug test_generate_zed_json (release)\"",
				"\"program\": \"bin/debug/test_generate_zed_json\"",
				"\"program\": \"bin/release/test_generate_zed_json\"",
				"\"cwd\": \"${ZED_WORKTREE_ROOT}\"",
				"\"adapter\": \"CodeLLDB\"",
				"\"request\": \"launch\"",
			),
		},
	};

	// build the build EXE
	Test_RunProcess( &testScratch, buildEXEFilename, 0, true );

	// generate the json files
	{
		const char *buildArgs = Builder_FormatString( &testScratch, "%s --zed", buildEXEFilename );

		Test_RunProcess( &testScratch, buildArgs, 0, true );
	}

	// check each file has everything we asked for
	for ( uint32_t fileIndex = 0; fileIndex < BUILDER_COUNT_OF( expectedFiles ); fileIndex++ ) {
		expectedJSONFile_t *expectedFile = &expectedFiles[fileIndex];

		uint64_t fileSize = 0;
		uint8_t *fileData = Builder_ReadEntireFile( &testScratch, expectedFile->filename, &fileSize );

		TEMPER_CHECK_TRUE_M( fileData, "Failed to read \"%s\".  It should've been generated.\n", expectedFile->filename );

		if ( !fileData ) {
			continue;
		}

		// file data isnt null terminated
		const char *fileContents = Builder_FormatString( &testScratch, "%.*s", fileSize, (const char *) fileData );

		for ( builderStringChunk_t *chunk = expectedFile->expectedEntries.head; chunk; chunk = chunk->next ) {
			for ( uint32_t entryIndex = 0; entryIndex < chunk->count; entryIndex++ ) {
				const char *expectedEntry = chunk->items[entryIndex];

				TEMPER_CHECK_TRUE_M( Builder_StringContains( fileContents, expectedEntry ), "\"%s\" is missing expected entry: %s\n", expectedFile->filename, expectedEntry );
			}
		}
	}

	// make the test clean up after itself
	{
		for ( uint32_t fileIndex = 0; fileIndex < BUILDER_COUNT_OF( expectedFiles ); fileIndex++ ) {
			const char *filename = expectedFiles[fileIndex].filename;

			bool deleted = Test_DeleteFile( filename );

			TEMPER_CHECK_TRUE_M( deleted, "Failed to delete file \"%s\".  The tests should properly clean up after themselves.\n", filename );
		}

		bool deleted = Test_DeleteFolder( dotZedFolder );

		TEMPER_CHECK_TRUE_M( deleted, "Failed to delete folder \"%s\".  The tests should properly clean up after themselves.\n", dotZedFolder );
	}
}

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
	Test_RunProcess( &testScratch, buildEXEFilename, 0, true );

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

			Test_RunProcess( &testScratch, buildArgs, 0, true );
		}

		// clangd
		for ( builderStringChunk_t *chunk = sourceFiles.head; chunk; chunk = chunk->next ) {
			for ( uint32_t sourceFileIndex = 0; sourceFileIndex < chunk->count; sourceFileIndex++ ) {
				const char *clangdArgs = Builder_FormatString( &testScratch, "%s --check=%s", clangdPath, chunk->items[sourceFileIndex] );

				Test_RunProcess( &testScratch, clangdArgs, 0, false );
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

			Test_RunProcess( &testScratch, clangTidyArgs, 0, false );
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
