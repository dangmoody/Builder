/*
===========================================================================

Builder

Copyright (c) 2025 Dan Moody

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

===========================================================================
*/

#include "builder_local.h"

#include "core/include/core_types.h"
#include "core/include/string_builder.h"
#include "core/include/string_helpers.h"
#include "core/include/array.inl"
#include "core/include/file.h"
#include "core/include/paths.h"
#include "core/include/typecast.inl"
#include "core/include/temp_storage.h"
#include "core/include/hash.h"

#ifdef _WIN64
#include <Shlwapi.h>
#endif


// some project type guids are pre-determined by visual studio
#define VISUAL_STUDIO_CPP_PROJECT_TYPE_GUID		"8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942"	// c++ project
#define VISUAL_STUDIO_FOLDER_PROJECT_TYPE_GUID	"2150E333-8FDC-42A3-9474-1A3956D46DE8"	// project folder

struct visualStudioFileFilter_t {
	const char* pathFromVisualStudioToRootFolder;
	const char* folder;	// relative from the root code folder that it was found in
	const char* filename;
};

static void GetAllVisualStudioFiles_r( const char* basePath, const char* subfolder, const std::vector<const char*> fileExtensions, Array<visualStudioFileFilter_t>& outFilterFiles ) {
	const char* fullSearchPath = NULL;

	if ( string_ends_with( basePath, "\\" ) || string_ends_with( basePath, "/" ) ) {
		if ( subfolder ) {
			fullSearchPath = tprintf( "%s%s\\*", basePath, subfolder );
		} else {
			fullSearchPath = tprintf( "%s*", basePath );
		}
	} else {
		if ( subfolder ) {
			fullSearchPath = tprintf( "%s\\%s\\*", basePath, subfolder );
		} else {
			fullSearchPath = tprintf( "%s\\*", basePath );
		}
	}

	FileInfo fileInfo;
	File file = file_find_first( fullSearchPath, &fileInfo );

	do {
		if ( string_equals( fileInfo.filename, "." ) ) {
			continue;
		}

		if ( string_equals( fileInfo.filename, ".." ) ) {
			continue;
		}

		if ( fileInfo.is_directory ) {
			const char* folderName = NULL;
			if ( subfolder ) {
				folderName = tprintf( "%s\\%s", subfolder, fileInfo.filename );
			} else {
				folderName = tprintf( "%s", fileInfo.filename );
			}

			GetAllVisualStudioFiles_r( basePath, folderName, fileExtensions, outFilterFiles );
		} else {
			const char* filename = tprintf( "%s", fileInfo.filename );

			For ( u64, fileExtensionIndex, 0, fileExtensions.size() ) {
				if ( string_ends_with( filename, fileExtensions[fileExtensionIndex] ) ) {
					outFilterFiles.add( { NULL, subfolder, filename } );
					break;
				}
			}
		}
	} while ( file_find_next( &file, &fileInfo ) );
}

// data layout comes from: https://learn.microsoft.com/en-us/windows/win32/api/guiddef/ns-guiddef-guid
static const char* CreateVisualStudioGuid() {
	GUID guid;
	HRESULT hr = CoCreateGuid( &guid );
	assert( hr == S_OK );

	return tprintf( "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] );
}

bool8 GenerateVisualStudioSolution( buildContext_t* context, BuilderOptions* options, const char* userConfigSourceFilename, const char* userConfigBuildDLLFilename, const bool8 verbose ) {
	assert( context );
	assert( context->inputFile );
	assert( context->inputFilePath );
	assert( context->dotBuilderFolder );
	assert( context->buildInfoFilename );
	assert( options );

	Array<const char*> projectFolders;
	Array<u64> projectFoldersHashes;

	struct projectParentMapping_t {
		u64	projectIndex;
		u64	parentIndex;
	};

	Array<projectParentMapping_t> projectParentMappings;

	// TODO(DM):
	//	18/11/2024: dont use abs path here
	//	22/02/2025: when Core gets a string data structure use that here, because this is just as horrible as the other times you do this
	{
		u64 buildInfoFilenameLength = strlen( context->inputFilePath ) + strlen( "\\.builder\\" ) + strlen( options->solution.name ) + strlen( BUILD_INFO_FILE_EXTENSION );

		char* buildInfoFilename = cast( char*, mem_alloc( buildInfoFilenameLength + 1 ) );	// + 1 for null terminator
		sprintf( buildInfoFilename, "%s\\.builder\\%s%s", context->inputFilePath, options->solution.name, BUILD_INFO_FILE_EXTENSION );
		buildInfoFilename[buildInfoFilenameLength] = 0;

		context->buildInfoFilename = buildInfoFilename;
	}

	// validate the solution
	{
		if ( options->solution.name == NULL ) {
			error( "You never set the name of the solution.  I need that.\n" );
			return false;
		}

		if ( options->solution.platforms.size() < 1 ) {
			error( "You must set at least one platform when generating a Visual Studio Solution.\n" );
			return false;
		}

		if ( options->solution.projects.size() < 1 ) {
			error( "As well as a Solution, you must also generate at least one Visual Studio Project to go with it.\n" );
			return false;
		}

		// validate platforms
		// turns out visual studio REALLY cares what the names of the platforms are
		// if you specify "Win32" or "x64" as a platform name then VS is able to generate defaults for your project which include things like Windows SDK directories, and your PATH
		{
			bool8 foundDefaultPlatformName = false;

			const char* defaultPlatformNames[] = {
				"Win32",
				"x64"
			};

			For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
				const char* platform = options->solution.platforms[platformIndex];

				For ( u64, defaultPlatformIndex, 0, count_of( defaultPlatformNames ) ) {
					const char* defaultPlatformName = defaultPlatformNames[defaultPlatformIndex];

					if ( string_equals( platform, defaultPlatformName ) ) {
						foundDefaultPlatformName = true;
						break;
					}
				}
			}

			if ( !foundDefaultPlatformName ) {
				StringBuilder error = {};
				string_builder_reset( &error );
				string_builder_appendf( &error, "None of your platform names are any of the Visual Studio recognized defaults:\n" );
				For ( u64, platformIndex, 0, count_of( defaultPlatformNames ) ) {
					string_builder_appendf( &error, "\t- %s\n", defaultPlatformNames[platformIndex] );
				}
				string_builder_appendf( &error, "Visual Studio relies on those specific names in order to generate fields like \"Executable Path\" properly (for example).\n" );
				string_builder_appendf( &error, "Builder will still generate the solution, but know that not setting at least one platform name to any of these defaults will cause certain fields in the property pages of your Visual Studio project to not be correct.\n" );
				string_builder_appendf( &error, "You have been warned.\n" );

				warning( string_builder_to_string( &error ) );
			}
		}
	}

	const char* visualStudioProjectFilesPath = NULL;
	if ( options->solution.path ) {
		visualStudioProjectFilesPath = tprintf( "%s\\%s", context->inputFilePath, options->solution.path );
	} else {
		visualStudioProjectFilesPath = context->inputFilePath;
	}
	visualStudioProjectFilesPath = paths_canonicalise_path( visualStudioProjectFilesPath );

	const char* solutionFilename = tprintf( "%s\\%s.sln", visualStudioProjectFilesPath, options->solution.name );

	// get relative path from visual studio to the input file
	char* pathFromSolutionToInputFile = cast( char*, mem_temp_alloc( MAX_PATH * sizeof( char ) ) );
	memset( pathFromSolutionToInputFile, 0, MAX_PATH * sizeof( char ) );
	PathRelativePathTo( pathFromSolutionToInputFile, solutionFilename, FILE_ATTRIBUTE_NORMAL, paths_fix_slashes( context->inputFilePath ), FILE_ATTRIBUTE_DIRECTORY );
	assert( pathFromSolutionToInputFile != NULL || !string_equals( pathFromSolutionToInputFile, "" ) );

	// give each project a guid
	Array<const char*> projectGuids;
	projectGuids.resize( options->solution.projects.size() );

	For ( u64, guidIndex, 0, projectGuids.count ) {
		projectGuids[guidIndex] = CreateVisualStudioGuid();
	}

	if ( !folder_create_if_it_doesnt_exist( visualStudioProjectFilesPath ) ) {
		errorCode_t errorCode = GetLastErrorCode();
		error( "Failed to create the Visual Studio Solution folder.  Error code: " ERROR_CODE_FORMAT "\n", errorCode );

		return false;
	}

	auto WriteStringBuilderToFile = []( StringBuilder* stringBuilder, const char* filename ) -> bool8 {
		const char* msg = string_builder_to_string( stringBuilder );
		const u64 msgLength = strlen( msg );
		bool8 written = file_write_entire( filename, msg, msgLength );

		if ( !written ) {
			errorCode_t errorCode = GetLastErrorCode();
			error( "Failed to write \"%s\": " ERROR_CODE_FORMAT ".\n", filename, errorCode );

			return false;
		}

		return true;
	};

	// generate each .vcxproj
	For ( u64, projectIndex, 0, options->solution.projects.size() ) {
		VisualStudioProject* project = &options->solution.projects[projectIndex];

		// validate the project
		{
			if ( project->name == NULL ) {
				error( "There is a Visual Studio Project that doesn't have a name here.  You need to fill that in.\n" );
				return false;
			}

			if ( project->code_folders.size() == 0 ) {
				error( "No code folders were provided for project \"%s\".  You need at least one.\n", project->name );
				return false;
			}

			if ( project->file_extensions.size() == 0 ) {
				error( "No file extensions/file types were provided for project \"%s\".  You need at least one.\n", project->name );
				return false;
			}

			// validate each config
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig* config = &project->configs[configIndex];

				if ( config->name == NULL ) {
					error( "There is a config for project \"%s\" that doesn't have a name here.  You need to fill that in.\n", project->name );
					return false;
				}

				if ( config->options.name.empty() ) {
					error( "There is a config for project \"%s\" that doesn't have a name set in it's BuildConfig.  You need to fill that in.\n", project->name );
					return false;
				}

				if ( config->options.binary_type == BINARY_TYPE_EXE && config->options.binary_folder.empty() ) {
					error(
						"Build config \"%s\" is an executable, but you never specified an output directory when generating the Visual Studio project \"%s\", config \"%s\".\n"
						"Visual Studio needs this in order to know where to run the executable from when debugging.  You need to set this.\n"
						, config->options.name.c_str(), project->name, config->name
					);
					return false;
				}
			}
		}

		const char* projectNameNoFolder = paths_remove_path_from_file( project->name );

		// if the project name has a slash in it then the user wants that project to be in a folder
		// for example a project with the name "projects/games/shooter" means the user wants a project called "shooter" inside a folder called "games", which is in turn inside a folder called "projects"
		// so split the string between slashes, and create project folders for each unique folder name that we find
		{
			const char* fullFolderPath = paths_remove_file_from_path( project->name );

			if ( fullFolderPath ) {
				u64 fullFolderPathHash = hash_string( fullFolderPath, 0 );

				u64 folderIndex = U64_MAX;

				For ( u64, folderHashIndex, 0, projectFoldersHashes.count ) {
					if ( projectFoldersHashes[folderHashIndex] == fullFolderPathHash ) {
						folderIndex = folderHashIndex;
						break;
					}
				}

				if ( folderIndex == U64_MAX ) {
					projectFoldersHashes.add( fullFolderPathHash );

					const char* folderStart = fullFolderPath;
					const char* previousFolder = NULL;

					u64 numFolders = 0;

					while ( folderStart ) {
						numFolders++;

						const char* folderEnd = GetSlashInPath( folderStart );

						if ( !folderEnd ) {
							folderEnd = folderStart + strlen( folderStart );
						}

						u64 folderNameLength = cast( u64, folderEnd ) - cast( u64, folderStart );

						char* folderName = cast( char*, mem_temp_alloc( ( folderNameLength ) + 1 * sizeof( char ) ) );	// + 1 for null terminator
						strncpy( folderName, folderStart, folderNameLength );
						folderName[folderNameLength] = 0;

						projectFolders.add( folderName );
						projectGuids.add( CreateVisualStudioGuid() );

						folderIndex = projectFolders.count - 1;

						// if there was a previous folder then add a mapping from this folder to that parent
						if ( previousFolder ) {
							u64 mappingIndex = options->solution.projects.size() + folderIndex;

							projectParentMappings.add( { .projectIndex = mappingIndex, .parentIndex = mappingIndex - 1, } );
						}

						previousFolder = folderName;

						folderStart = GetSlashInPath( folderEnd );
					}
				}

				projectParentMappings.add( { .projectIndex = projectIndex, .parentIndex = options->solution.projects.size() + folderIndex } );
			}
		}

		// get all the files that the project will know about
		// the arrays in here get referred to multiple times throughout generating the files for the project
		Array<visualStudioFileFilter_t> sourceFiles;
		Array<visualStudioFileFilter_t> headerFiles;
		Array<visualStudioFileFilter_t> otherFiles;

		{
			For ( u64, folderIndex, 0, project->code_folders.size() ) {
				const char* folder = project->code_folders[folderIndex];

				const char* pathFromSolutionToCode = tprintf( "%s\\%s", pathFromSolutionToInputFile, folder );

				Array<visualStudioFileFilter_t> filterFiles;
				GetAllVisualStudioFiles_r( tprintf( "%s\\%s", context->inputFilePath, folder ), NULL, project->file_extensions, filterFiles );

				For ( u64, i, 0, filterFiles.count ) {
					visualStudioFileFilter_t* fileFilter = &filterFiles[i];

					fileFilter->pathFromVisualStudioToRootFolder = pathFromSolutionToCode;

					if ( FileIsSourceFile( fileFilter->filename ) ) {
						sourceFiles.add( *fileFilter );
					} else if ( FileIsHeaderFile( fileFilter->filename ) ) {
						headerFiles.add( *fileFilter );
					} else {
						otherFiles.add( *fileFilter );
					}
				}
			}
		}

		// .vcxproj
		{
			const char* projectPath = tprintf( "%s\\%s.vcxproj", visualStudioProjectFilesPath, projectNameNoFolder );

			printf( "Generating %s ... ", projectPath );

			StringBuilder vcxprojContent = {};
			string_builder_reset( &vcxprojContent );

			string_builder_appendf( &vcxprojContent, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
			string_builder_appendf( &vcxprojContent, "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

			// generate every single config and platform pairing
			{
				string_builder_appendf( &vcxprojContent, "\t<ItemGroup Label=\"ProjectConfigurations\">\n" );
				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig* config = &project->configs[configIndex];

					For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
						const char* platform = options->solution.platforms[platformIndex];

						// TODO: Alternative targets
						string_builder_appendf( &vcxprojContent, "\t\t<ProjectConfiguration Include=\"%s|%s\">\n", config->name, platform );
						string_builder_appendf( &vcxprojContent, "\t\t\t<Configuration>%s</Configuration>\n", config->name );
						string_builder_appendf( &vcxprojContent, "\t\t\t<Platform>%s</Platform>\n", platform );
						string_builder_appendf( &vcxprojContent, "\t\t</ProjectConfiguration>\n" );
					}
				}
				string_builder_appendf( &vcxprojContent, "\t</ItemGroup>\n" );
			}

			// project globals
			{
				string_builder_appendf( &vcxprojContent, "\t<PropertyGroup Label=\"Globals\">\n" );
				string_builder_appendf( &vcxprojContent, "\t\t<VCProjectVersion>17.0</VCProjectVersion>\n" );
				string_builder_appendf( &vcxprojContent, "\t\t<ProjectGuid>{%s}</ProjectGuid>\n", projectGuids[projectIndex] );
				string_builder_appendf( &vcxprojContent, "\t\t<IgnoreWarnCompileDuplicatedFilename>true</IgnoreWarnCompileDuplicatedFilename>\n" );
				string_builder_appendf( &vcxprojContent, "\t\t<Keyword>MakeFileProj</Keyword>\n" );
				string_builder_appendf( &vcxprojContent, "\t</PropertyGroup>\n" );
			}

			string_builder_appendf( &vcxprojContent, "\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n" );

			// for each config and platform, define config type, toolset, out dir, and intermediate dir
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig* config = &project->configs[configIndex];

				const char* fullBinaryName = BuildConfig_GetFullBinaryName( &config->options );

				const char* from = solutionFilename;
				const char* to = tprintf( "%s\\%s", context->inputFilePath, fullBinaryName );
				to = paths_canonicalise_path( to );

				char* pathFromSolutionToBinary = cast( char*, mem_temp_alloc( MAX_PATH * sizeof( char ) ) );
				memset( pathFromSolutionToBinary, 0, MAX_PATH * sizeof( char ) );
				PathRelativePathTo( pathFromSolutionToBinary, from, FILE_ATTRIBUTE_NORMAL, to, FILE_ATTRIBUTE_DIRECTORY );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
				pathFromSolutionToBinary = cast( char*, paths_remove_file_from_path( pathFromSolutionToBinary ) );
#pragma clang diagnostic pop

				For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
					const char* platform = options->solution.platforms[platformIndex];

					string_builder_appendf( &vcxprojContent, "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\" Label=\"Configuration\">\n", config->name, platform );
					string_builder_appendf( &vcxprojContent, "\t\t<ConfigurationType>Makefile</ConfigurationType>\n" );
					string_builder_appendf( &vcxprojContent, "\t\t<UseDebugLibraries>false</UseDebugLibraries>\n" );
					string_builder_appendf( &vcxprojContent, "\t\t<PlatformToolset>v143</PlatformToolset>\n" );

					string_builder_appendf( &vcxprojContent, "\t\t<OutDir>%s</OutDir>\n", pathFromSolutionToBinary );
					string_builder_appendf( &vcxprojContent, "\t\t<IntDir>%s\\intermediate</IntDir>\n", config->options.binary_folder.c_str() );
					string_builder_appendf( &vcxprojContent, "\t</PropertyGroup>\n" );
				}
			}

			string_builder_appendf( &vcxprojContent, "\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n" );

			// not sure what this is or why we need this one but visual studio seems to want it
			string_builder_appendf( &vcxprojContent, "\t<ImportGroup Label=\"ExtensionSettings\">\n" );
			string_builder_appendf( &vcxprojContent, "\t</ImportGroup>\n" );

			// for each config and platform, import the property sheets that visual studio requires
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig* config = &project->configs[configIndex];

				For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
					const char* platform = options->solution.platforms[platformIndex];

					string_builder_appendf( &vcxprojContent, "\t<ImportGroup Label=\"PropertySheets\" Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\">\n", config->name, platform );
					string_builder_appendf( &vcxprojContent, "\t\t<Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists(\'$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\')\" Label=\"LocalAppDataPlatform\" />\n" );
					string_builder_appendf( &vcxprojContent, "\t</ImportGroup>\n" );
				}
			}

			// not sure what this is or why we need this one but visual studio seems to want it
			string_builder_appendf( &vcxprojContent, "\t<PropertyGroup Label=\"UserMacros\" />\n" );
		
			// for each config and platform, set the following:
			//	external include paths
			//	external library paths
			//	output path
			//	build command
			//	rebuild command
			//	clean command
			//	preprocessor definitions
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig* config = &project->configs[configIndex];

				For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
					const char* platform = options->solution.platforms[platformIndex];

					string_builder_appendf( &vcxprojContent, "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\">\n", config->name, platform );

					// external include paths
					string_builder_appendf( &vcxprojContent, "\t\t<ExternalIncludePath>" );
					For ( u64, includePathIndex, 0, config->options.additional_includes.size() ) {
						const char* additionalInclude = config->options.additional_includes[includePathIndex].c_str();

						if ( paths_is_path_absolute( additionalInclude ) ) {
							string_builder_appendf( &vcxprojContent, "%s;", config->options.additional_includes[includePathIndex].c_str() );
						} else {
							string_builder_appendf( &vcxprojContent, "%s\\%s;", pathFromSolutionToInputFile, config->options.additional_includes[includePathIndex].c_str() );
						}
					}
					string_builder_appendf( &vcxprojContent, "$(ExternalIncludePath)" );
					string_builder_appendf( &vcxprojContent, "</ExternalIncludePath>\n" );

					// external library paths
					string_builder_appendf( &vcxprojContent, "\t\t<LibraryPath>" );
					For ( u64, libPathIndex, 0, config->options.additional_lib_paths.size() ) {
						const char* additionalLibPath = config->options.additional_lib_paths[libPathIndex].c_str();

						if ( paths_is_path_absolute( additionalLibPath ) ) {
							string_builder_appendf( &vcxprojContent, "%s;", additionalLibPath );
						} else {
							string_builder_appendf( &vcxprojContent, "%s\\%s;", pathFromSolutionToInputFile, additionalLibPath );
						}
					}
					string_builder_appendf( &vcxprojContent, "$(LibraryPath)" );
					string_builder_appendf( &vcxprojContent, "</LibraryPath>\n" );

					// output path
					string_builder_appendf( &vcxprojContent, "\t\t<NMakeOutput>%s</NMakeOutput>\n", config->options.binary_folder.c_str() );

					const char* buildInfoFileRelative = tprintf( "%s\\.builder\\%s%s", pathFromSolutionToInputFile, options->solution.name, BUILD_INFO_FILE_EXTENSION );

					const char* fullConfigName = config->options.name.c_str();

					string_builder_appendf( &vcxprojContent, "\t\t<NMakeBuildCommandLine>%s\\builder.exe %s %s%s</NMakeBuildCommandLine>\n", paths_get_app_path(), buildInfoFileRelative, ARG_CONFIG, fullConfigName );
					string_builder_appendf( &vcxprojContent, "\t\t<NMakeReBuildCommandLine>%s\\builder.exe %s %s%s</NMakeReBuildCommandLine>\n", paths_get_app_path(), buildInfoFileRelative, ARG_CONFIG, fullConfigName );

					string_builder_appendf( &vcxprojContent, "\t\t<NMakeCleanCommandLine>%s\\builder.exe --nuke %s</NMakeCleanCommandLine>\n", paths_get_app_path(), config->options.binary_folder.c_str() );

					// preprocessor definitions
					string_builder_appendf( &vcxprojContent, "\t\t<NMakePreprocessorDefinitions>" );
					For ( u64, definitionIndex, 0, config->options.defines.size() ) {
						string_builder_appendf( &vcxprojContent, tprintf( "%s;", config->options.defines[definitionIndex] ) );
					}
					string_builder_appendf( &vcxprojContent, "$(NMakePreprocessorDefinitions)" );
					string_builder_appendf( &vcxprojContent, "</NMakePreprocessorDefinitions>\n" );

					string_builder_appendf( &vcxprojContent, "\t</PropertyGroup>\n" );
				}
			}

			string_builder_appendf( &vcxprojContent, "\t<ItemDefinitionGroup>\n" );
			string_builder_appendf( &vcxprojContent, "\t</ItemDefinitionGroup>\n" );

			// tell visual studio what files we have in this project
			// this is typically done via a filter (E.G: src/*.cpp)
			{
				auto WriteFilterFilesToVcxproj = []( StringBuilder* stringBuilder, const Array<visualStudioFileFilter_t>& files, const char* tag ) {
					if ( files.count == 0 ) {
						return;
					}

					string_builder_appendf( stringBuilder, "\t<ItemGroup>\n" );

					For ( u64, fileIndex, 0, files.count ) {
						const visualStudioFileFilter_t* file = &files[fileIndex];

						if ( file->folder ) {
							string_builder_appendf( stringBuilder, "\t\t<%s Include=\"%s\\%s\\%s\" />\n", tag, file->pathFromVisualStudioToRootFolder, file->folder, file->filename );
						} else {
							string_builder_appendf( stringBuilder, "\t\t<%s Include=\"%s\\%s\" />\n", tag, file->pathFromVisualStudioToRootFolder, file->filename );
						}
					}

					string_builder_appendf( stringBuilder, "\t</ItemGroup>\n" );
				};

				WriteFilterFilesToVcxproj( &vcxprojContent, sourceFiles, "ClCompile" );
				WriteFilterFilesToVcxproj( &vcxprojContent, headerFiles, "ClInclude" );
				WriteFilterFilesToVcxproj( &vcxprojContent, otherFiles, "None" );
			}

			string_builder_appendf( &vcxprojContent, "\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n" );

			// not sure what this is or why we need this one but visual studio seems to want it
			string_builder_appendf( &vcxprojContent, "\t<ImportGroup Label=\"ExtensionTargets\">\n" );
			string_builder_appendf( &vcxprojContent, "\t</ImportGroup>\n" );

			string_builder_appendf( &vcxprojContent, "</Project>\n" );

			if ( !WriteStringBuilderToFile( &vcxprojContent, projectPath ) ) {
				return false;
			}

			printf( "Done\n" );
		}

		// .vcxproj.user
		{
			const char* projectPath = tprintf( "%s\\%s.vcxproj.user", visualStudioProjectFilesPath, projectNameNoFolder );

			printf( "Generating %s ... ", projectPath );

			StringBuilder vcxprojContent = {};
			string_builder_reset( &vcxprojContent );

			string_builder_appendf( &vcxprojContent, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
			string_builder_appendf( &vcxprojContent, "<Project ToolsVersion=\"Current\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

			string_builder_appendf( &vcxprojContent, "\t<PropertyGroup>\n" );
			string_builder_appendf( &vcxprojContent, "\t\t<ShowAllFiles>false</ShowAllFiles>\n" );
			string_builder_appendf( &vcxprojContent, "\t</PropertyGroup>\n" );

			// for each config and platform, generate the debugger settings
			{
				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig* config = &project->configs[configIndex];

					const char* fullBinaryName = BuildConfig_GetFullBinaryName( &config->options );

					const char* from = solutionFilename;
					const char* to = tprintf( "%s\\%s", context->inputFilePath, fullBinaryName );
					to = paths_canonicalise_path( to );

					char* pathFromSolutionToBinary = cast( char*, mem_temp_alloc( MAX_PATH * sizeof( char ) ) );
					memset( pathFromSolutionToBinary, 0, MAX_PATH * sizeof( char ) );
					PathRelativePathTo( pathFromSolutionToBinary, from, FILE_ATTRIBUTE_NORMAL, to, FILE_ATTRIBUTE_DIRECTORY );

					For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
						const char* platform = options->solution.platforms[platformIndex];

						string_builder_appendf( &vcxprojContent, "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\">\n", config->name, platform );
						string_builder_appendf( &vcxprojContent, "\t\t<DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>\n" );	// TODO(DM): do want to include the other debugger types?
						string_builder_appendf( &vcxprojContent, "\t\t<LocalDebuggerDebuggerType>Auto</LocalDebuggerDebuggerType>\n" );
						string_builder_appendf( &vcxprojContent, "\t\t<LocalDebuggerAttach>false</LocalDebuggerAttach>\n" );
						string_builder_appendf( &vcxprojContent, "\t\t<LocalDebuggerCommand>%s</LocalDebuggerCommand>\n", pathFromSolutionToBinary );
						string_builder_appendf( &vcxprojContent, "\t\t<LocalDebuggerWorkingDirectory>$(SolutionDir)</LocalDebuggerWorkingDirectory>\n" );

						// if debugger arguments were specified, put those in
						if ( config->debugger_arguments.size() > 0 ) {
							string_builder_appendf( &vcxprojContent, "\t\t<LocalDebuggerCommandArguments>\n" );
							For ( u64, argIndex, 0, config->debugger_arguments.size() ) {
								string_builder_appendf( &vcxprojContent, "%s ", config->debugger_arguments[argIndex] );
							}
							string_builder_appendf( &vcxprojContent, "</LocalDebuggerCommandArguments>\n" );
						}

						string_builder_appendf( &vcxprojContent, "\t</PropertyGroup>\n" );
					}
				}
			}

			string_builder_appendf( &vcxprojContent, "</Project>\n" );

			if ( !WriteStringBuilderToFile( &vcxprojContent, projectPath ) ) {
				return false;
			}

			printf( "Done\n" );
		}

		// .vcxproj.filter
		{
			const char* projectPath = tprintf( "%s\\%s.vcxproj.filters", visualStudioProjectFilesPath, projectNameNoFolder );

			printf( "Generating %s ... ", projectPath );

			StringBuilder vcxprojContent = {};
			string_builder_reset( &vcxprojContent );

			string_builder_appendf( &vcxprojContent, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
			string_builder_appendf( &vcxprojContent, "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

			string_builder_appendf( &vcxprojContent, "\t<ItemGroup>\n" );

			// write all filter guids
			For ( u64, folderIndex, 0, project->code_folders.size() ) {
				const char* folder = project->code_folders[folderIndex];

				Array<const char*> subfolders;
				GetAllSubfolders_r( tprintf( "%s\\%s", context->inputFilePath, folder ), NULL, &subfolders );

				For ( u64, subfolderIndex, 0, subfolders.count ) {
					subfolders[subfolderIndex] = paths_fix_slashes( subfolders[subfolderIndex] );
				}

				For ( u64, subfolderIndex, 0, subfolders.count ) {
					const char* subfolder = subfolders[subfolderIndex];

					const char* guid = CreateVisualStudioGuid();

					string_builder_appendf( &vcxprojContent, "\t\t<Filter Include=\"%s\">\n", subfolder );
					string_builder_appendf( &vcxprojContent, "\t\t\t<UniqueIdentifier>{%s}</UniqueIdentifier>\n", guid );
					string_builder_appendf( &vcxprojContent, "\t\t</Filter>\n" );
				}
			}

			string_builder_appendf( &vcxprojContent, "\t</ItemGroup>\n" );

			// now put all files in the filter
			// visual studio requires that we list each file by type
			{
				auto WriteFileFilters = []( StringBuilder* stringBuilder, const Array<visualStudioFileFilter_t>& fileFilters, const char* tag ) {
					if ( fileFilters.count == 0 ) {
						return;
					}

					string_builder_appendf( stringBuilder, "\t<ItemGroup>\n" );

					For ( u64, fileIndex, 0, fileFilters.count ) {
						const visualStudioFileFilter_t* file = &fileFilters[fileIndex];

						if ( file->folder == NULL ) {
							string_builder_appendf( stringBuilder, "\t\t<%s Include=\"%s\\%s\" />\n", tag, file->pathFromVisualStudioToRootFolder, file->filename );
						} else {
							string_builder_appendf( stringBuilder, "\t\t<%s Include=\"%s\\%s\\%s\">\n", tag, file->pathFromVisualStudioToRootFolder, file->folder, file->filename );
							string_builder_appendf( stringBuilder, "\t\t\t<Filter>%s</Filter>\n", file->folder );
							string_builder_appendf( stringBuilder, "\t\t</%s>\n", tag );
						}
					}

					string_builder_appendf( stringBuilder, "\t</ItemGroup>\n" );
				};

				WriteFileFilters( &vcxprojContent, sourceFiles, "ClCompile" );
				WriteFileFilters( &vcxprojContent, headerFiles, "ClInclude" );
				WriteFileFilters( &vcxprojContent, otherFiles, "None" );
			}

			string_builder_appendf( &vcxprojContent, "</Project>" );

			if ( !WriteStringBuilderToFile( &vcxprojContent, projectPath ) ) {
				return false;
			}
		}

		printf( "Done\n" );
	}

	// .sln
	{
		printf( "Generating %s ... ", solutionFilename );

		StringBuilder slnContent = {};
		string_builder_reset( &slnContent );

		string_builder_appendf( &slnContent, "\n" );
		string_builder_appendf( &slnContent, "Microsoft Visual Studio Solution File, Format Version 12.00\n" );
		string_builder_appendf( &slnContent, "# Visual Studio Version 17\n" );
		string_builder_appendf( &slnContent, "VisualStudioVersion = 17.7.34202.233\n" );		// TODO(DM): how do we query windows for this?
		string_builder_appendf( &slnContent, "MinimunVisualStudioVersion = 10.0.40219.1\n" );	// TODO(DM): how do we query windows for this?

		// project GUIDs
		For ( u64, projectIndex, 0, options->solution.projects.size() ) {
			VisualStudioProject* project = &options->solution.projects[projectIndex];

			const char* projectName = paths_remove_path_from_file( project->name );

			string_builder_appendf( &slnContent, "Project(\"{%s}\") = \"%s\", \"%s.vcxproj\", \"{%s}\"\n", VISUAL_STUDIO_CPP_PROJECT_TYPE_GUID, projectName, projectName, projectGuids[projectIndex] );
			string_builder_appendf( &slnContent, "EndProject\n" );
		}

		// project folder GUIDs
		For ( u64, projectFolderIndex, 0, projectFolders.count ) {
			const char* folderName = projectFolders[projectFolderIndex];

			u64 folderGuidIndex = options->solution.projects.size() + projectFolderIndex;

			string_builder_appendf( &slnContent, "Project(\"{%s}\") = \"%s\", \"%s\", \"{%s}\"\n", VISUAL_STUDIO_FOLDER_PROJECT_TYPE_GUID, folderName, folderName, projectGuids[folderGuidIndex] );
			string_builder_appendf( &slnContent, "EndProject\n" );
		}

		string_builder_appendf( &slnContent, "Global\n" );
		{
			// which config|platform maps to which config|platform?
			string_builder_appendf( &slnContent, "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n" );
			For ( u64, projectIndex, 0, options->solution.projects.size() ) {
				VisualStudioProject* project = &options->solution.projects[projectIndex];

				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig* config = &project->configs[configIndex];

					For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
						const char* platform = options->solution.platforms[platformIndex];

						string_builder_appendf( &slnContent, tprintf( "\t\t%s|%s = %s|%s\n", config->name, platform, config->name, platform ) );
					}
				}
			}
			string_builder_appendf( &slnContent, "\tEndGlobalSection\n" );

			// which project config|platform is active?
			string_builder_appendf( &slnContent, "\tGlobalSection(SolutionConfigurationPlatforms) = postSolution\n" );
			For ( u64, projectIndex, 0, options->solution.projects.size() ) {
				VisualStudioProject* project = &options->solution.projects[projectIndex];

				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig* config = &project->configs[configIndex];

					For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
						const char* platform = options->solution.platforms[platformIndex];

						const char* projectGuid = projectGuids[projectIndex];

						// TODO: the first config and platform in this line are actually the ones that the PROJECT has, not the SOLUTION
						// but we dont use those, and we should
						string_builder_appendf( &slnContent, "\t\t{%s}.%s|%s.ActiveCfg = %s|%s\n", projectGuid, config->name, platform, config->name, platform );
						string_builder_appendf( &slnContent, "\t\t{%s}.%s|%s.Build.0 = %s|%s\n", projectGuid, config->name, platform, config->name, platform );
					}
				}
			}
			string_builder_appendf( &slnContent, "\tEndGlobalSection\n" );

			// tell visual studio to not hide the solution node in the Solution Explorer
			// why would you ever want it to be hidden?
			string_builder_appendf( &slnContent, "\tGlobalSection(SolutionProperties) = preSolution\n" );
			string_builder_appendf( &slnContent, "\t\tHideSolutionNode = FALSE\n" );
			string_builder_appendf( &slnContent, "\tEndGlobalSection\n" );

			// which projects are in which project folders (if any)?
			string_builder_appendf( &slnContent, "\tGlobalSection(NestedProjects) = preSolution\n" );
			For ( u64, projectIndex, 0, projectParentMappings.count ) {
				projectParentMapping_t* mapping = &projectParentMappings[projectIndex];

				string_builder_appendf( &slnContent, "\t\t{%s} = {%s}\n", projectGuids[mapping->projectIndex], projectGuids[mapping->parentIndex] );
			}
			string_builder_appendf( &slnContent, "\tEndGlobalSection\n" );

			//const char* solutionGUID = CreateVisualStudioGuid();

			//// we need to tell visual studio what the GUID of the solution is, apparently
			//// and we also need to do it in this really roundabout way...for some reason
			//string_builder_appendf( &slnContent, "\tGlobalSection(ExtensibilityGlobals) = postSolution\n" );
			//string_builder_appendf( &slnContent, "\t\tSolutionGuid = {%s}\n", solutionGUID );
			//string_builder_appendf( &slnContent, "\tEndGlobalSection\n" );
		}
		string_builder_appendf( &slnContent, "EndGlobal\n" );

		if ( !WriteStringBuilderToFile( &slnContent, solutionFilename ) ) {
			return false;
		}

		printf( "Done\n\n" );
	}

	// generate .build_info file
	{
		For ( u64, configIndex, 0, options->configs.size() ) {
			BuildConfig_AddDefaults( &options->configs[configIndex] );
		}

		BuildInfo_Write( context, options->configs, userConfigSourceFilename, userConfigBuildDLLFilename, verbose );
	}

	return true;
}