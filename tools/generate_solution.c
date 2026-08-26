// run: clang -o generate_solution.exe generate_solution.c to make the EXE

#define BUILDER_IMPLEMENTATION
#include "../builder.h"

#define BUILDER_VISUAL_STUDIO_IMPLEMENTATION
#include "../builder_visual_studio.h"

typedef struct testFolderList_t {
	const char	**names;
	uint32_t	count;
} testFolderList_t;

static void OnTestFolderFound( arena_t *resultsArena, fileInfo_t *fileInfo, void *data ) {
	if ( !fileInfo->isDirectory ) {
		return;
	}

	// dont generate a test for temper
	if ( Builder_StringEquals( fileInfo->filename, "temper" ) ) {
		return;
	}

	testFolderList_t *testFolders = (testFolderList_t *) data;

	testFolders->names = Builder_ArenaRealloc( resultsArena, testFolders->names, const char *, testFolders->count, testFolders->count + 1 );
	testFolders->names[testFolders->count] = Builder_FormatString( resultsArena, "%s", fileInfo->filename );
	testFolders->count++;
}

int main( int argc, char **argv ) {
	BuilderOptions options = { 0 };

	arena_t arena = { 0 };

	BuildConfig *builderConfig = CreateBuildConfig( &options );
	*builderConfig = (BuildConfig) {
		.name			= "builder",
		.binaryType		= BINARY_TYPE_STATIC_LIBRARY,
		.sourceFiles	= MakeStringList( "../*.h" ),
	};

	VisualStudioConfig *builderVsConfigs = Builder_ArenaAlloc( &arena, VisualStudioConfig, 2 );
	builderVsConfigs[0] = (VisualStudioConfig) { .name = "Debug",   .config = builderConfig };
	builderVsConfigs[1] = (VisualStudioConfig) { .name = "Release", .config = builderConfig, .additionalBuildArgs = MakeStringList( "--release" ) };

	testFolderList_t testFolders = { 0 };
	Builder_VisitFiles( &arena, "../tests/", BUILDER_FILE_VISIT_FOLDERS, OnTestFolderFound, &testFolders );

	uint32_t projectsCount = testFolders.count + 1;
	VisualStudioProject *projects = Builder_ArenaAlloc( &arena, VisualStudioProject, projectsCount );

	projects[0] = (VisualStudioProject) {
		.name			= "builder",
		.configs		= builderVsConfigs,
		.configsCount	= 2,
	};

	for ( uint32_t testIndex = 0; testIndex < testFolders.count; testIndex++ ) {
		const char *testName = testFolders.names[testIndex];

		BuildConfig *testConfig = CreateBuildConfig( &options );
		*testConfig = (BuildConfig) {
			.name			= testName,
			.binaryType		= BINARY_TYPE_STATIC_LIBRARY,
			.sourceFiles	= MakeStringList(
				Config_FormatString( "../tests/%s/**/*.c", testName ),
				Config_FormatString( "../tests/%s/**/*.h", testName )
			),
		};

		VisualStudioConfig *testVsConfigs = Builder_ArenaAlloc( &arena, VisualStudioConfig, 2 );
		testVsConfigs[0] = (VisualStudioConfig) { .name = "Debug",   .config = testConfig,                                                       .runFromDirectory = "$(SolutionDir).." };
		testVsConfigs[1] = (VisualStudioConfig) { .name = "Release", .config = testConfig, .additionalBuildArgs = MakeStringList( "--release" ), .runFromDirectory = "$(SolutionDir).." };

		projects[testIndex + 1] = (VisualStudioProject) {
			.name			= Builder_FormatString( &arena, "tests/%s", testName ),
			.configs		= testVsConfigs,
			.configsCount	= 2,
		};
	}

	VisualStudioSolution solution = {
		.name			= "builder",
		.path			= "visual_studio",
		.projects		= projects,
		.projectsCount	= projectsCount,
		.platforms		= MakeStringList( "x64" ),
	};

	return Builder_GenerateVisualStudioSolution( &options, &solution, argc, argv ) ? 0 : 1;
}
