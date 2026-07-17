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

#include "core/include/int_types.h"
#include "core/include/linear_allocator.h"
#include "core/include/string_builder.h"
#include "core/include/core_string.h"
#include "core/include/core_array.inl"
#include "core/include/file.h"
#include "core/include/paths.h"
#include "core/include/typecast.inl"
#include "core/include/temp_storage.h"
#include "core/include/hash.h"
#include "core/include/hashmap.h"
#include "core/include/debug.h"
#include "core/include/core_helpers.h"
#include "core/include/defer.h"

#if defined( _WIN32 )
#include <Shlwapi.h>
#elif defined( __linux__ )
#include <uuid/uuid.h>
#endif


// some project type guids are pre-determined by visual studio
#define VISUAL_STUDIO_CPP_PROJECT_TYPE_GUID		"8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942"	// c++ project
#define VISUAL_STUDIO_FOLDER_PROJECT_TYPE_GUID	"2150E333-8FDC-42A3-9474-1A3956D46DE8"	// project folder

struct visualStudioFileFilter_t {
	const char* filePath;
	String fileFilter;
};

// data layout comes from: https://learn.microsoft.com/en-us/windows/win32/api/guiddef/ns-guiddef-guid
// DM: I don't see a good enough argument that this is common enough that we want this in Core, currently, so I'm leaving this here for now
static const char *CreateVisualStudioGuid() {
#if defined( _WIN32 )
	GUID guid;
	HRESULT hr = CoCreateGuid( &guid );
	assert( hr == S_OK );

	return temp_printf( "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] );
#elif defined( __linux__ )
	const u64 guidStringLength = 37;
	char *guidString = cast( char *, mem_temp_alloc( guidStringLength * sizeof( char ) ) );

	uuid_t uuid;
	uuid_generate( uuid );
	uuid_unparse_upper( uuid, guidString );

	return guidString;
#endif
}

struct visualStudioNukeContext_t {
	Array<const char *>	fileExtensionsToCheck;
	String				dotVSFolder;
};

static void VS_DeleteOldProjectFilesCallback( const FileInfo *fileInfo, void *userData ) {
	visualStudioNukeContext_t *nukeContext = cast( visualStudioNukeContext_t *, userData );

	if ( fileInfo->is_directory && string_equals( fileInfo->filename, ".vs" ) ) {
		// TODO: DM: 09/05/2026: not sure we want to store this in temp storage
		nukeContext->dotVSFolder = string_alloc( mem_get_temp_storage(), fileInfo->full_filename, strlen( fileInfo->full_filename ) + 1 );
	}

	For ( u32, fileExtensionIndex, 0, nukeContext->fileExtensionsToCheck.count ) {
		if ( string_ends_with( fileInfo->full_filename, nukeContext->fileExtensionsToCheck[fileExtensionIndex] ) ) {
			LogVerbose( "Deleting file \"%s\"\n", fileInfo->full_filename );

			if ( file_delete( fileInfo->full_filename ) ) {
				break;
			} else {
				warning( "Failed to delete old Visual Studio file \"%s\" while deleting old Visual Studio files.  You will have to delete this one yourself.  Sorry.\n", fileInfo->full_filename );
			}
		}
	}
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"

bool8 GenerateVisualStudioSolution( buildContext_t *context, BuilderOptions *options ) {
	assert( context );
	assert( context->inputFile );
	assert( context->inputFilePath.data );
	assert( options );

	Array<char *> projectFolders;
	projectFolders.init( mem_get_temp_storage() );

	Hashmap *projectFolderIndices = hashmap_create( mem_get_temp_storage(), 1 );

	struct guidParentMapping_t {
		u64	guidIndex;
		u64	guidParentIndex;
	};

	Array<guidParentMapping_t> guidParentMappings;
	guidParentMappings.init( mem_get_temp_storage() );

	// validate the solution
	{
		bool8 validSolution = true;
		if ( options->solution.name.empty() ) {
			error( "You never set the name of the solution.  I need that.\n" );
			validSolution = false;
		}

		if ( options->solution.platforms.size() < 1 ) {
			error( "You must set at least one platform when generating a Visual Studio Solution.\n" );
			validSolution = false;
		}

		if ( options->solution.projects.size() < 1 ) {
			error( "As well as a Solution, you must also generate at least one Visual Studio Project to go with it.\n" );
			validSolution = false;
		}

		if ( !validSolution ) {
			return false;
		}

		// validate platforms
		// turns out visual studio REALLY cares what the names of the platforms are
		// if you specify "Win32" or "x64" as a platform name then VS is able to generate defaults for your project which include things like Windows SDK directories, and your PATH
		{
			bool8 foundDefaultPlatformName = false;

			const char *defaultPlatformNames[] = {
				"Win32",
				"x64",
				"linux-x64"
			};

			For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
				const char *platform = options->solution.platforms[platformIndex].c_str();

				For ( u64, defaultPlatformIndex, 0, count_of( defaultPlatformNames ) ) {
					const char *defaultPlatformName = defaultPlatformNames[defaultPlatformIndex];

					if ( string_equals( platform, defaultPlatformName ) ) {
						foundDefaultPlatformName = true;
						break;
					}
				}
			}

			if ( !foundDefaultPlatformName ) {
				// u64 pos = mem_temp_tell();
				// defer { mem_temp_rewind_to( pos ); };

				StringBuilder error = string_builder_create( context->allocator );
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

	const char *visualStudioProjectFilesPath = NULL;
	if ( !options->solution.path.empty() ) {
		visualStudioProjectFilesPath = temp_printf( "%s%c%s", context->inputFilePath.data, PATH_SEPARATOR, options->solution.path.c_str() );
	} else {
		visualStudioProjectFilesPath = context->inputFilePath.data;
	}
	//visualStudioProjectFilesPath = path_canonicalise( visualStudioProjectFilesPath );

	// delete old VS files if they exist
	// but keep the root because we're about to re-populate it
	{
		visualStudioNukeContext_t nukeContext = {};
		nukeContext.fileExtensionsToCheck.init( mem_get_temp_storage() );
		nukeContext.fileExtensionsToCheck.add( ".sln" );
		nukeContext.fileExtensionsToCheck.add( ".vcxproj" );
		nukeContext.fileExtensionsToCheck.add( ".vcxproj.user" );
		nukeContext.fileExtensionsToCheck.add( ".vcxproj.filters" );

		file_get_all_files_in_folder( visualStudioProjectFilesPath, FILE_VISIT_FILES | FILE_VISIT_FOLDERS, VS_DeleteOldProjectFilesCallback, &nukeContext );

		if ( nukeContext.dotVSFolder.data ) {
			NukeFolder( nukeContext.dotVSFolder.data, true, g_verbose );
		}
	}

	const char *solutionFilename = temp_printf( "%s%c%s.sln", visualStudioProjectFilesPath, PATH_SEPARATOR, options->solution.name.c_str() );

	// get relative path from visual studio to the input file
	String pathFromSolutionToInputFileStr = path_relative_path_to( context->allocator, visualStudioProjectFilesPath, context->inputFilePath.data );
	const char *pathFromSolutionToInputFile = pathFromSolutionToInputFileStr.data;
	assert( pathFromSolutionToInputFile != NULL && !string_equals( pathFromSolutionToInputFile, "" ) );

	// give each project a guid
	Array<const char *> projectGuids;
	projectGuids.init( mem_get_temp_storage() );
	projectGuids.resize( options->solution.projects.size() );

	For ( u64, guidIndex, 0, projectGuids.count ) {
		projectGuids[guidIndex] = CreateVisualStudioGuid();
	}

	if ( !folder_create_if_it_doesnt_exist( visualStudioProjectFilesPath ) ) {
		s32 errorCode = get_last_error_code();
		error( "Failed to create the Visual Studio Solution folder.  Error code: " ERROR_CODE_FORMAT "\n", errorCode );

		return false;
	}

	// generate each .vcxproj
	For ( u64, projectIndex, 0, options->solution.projects.size() ) {
		VisualStudioProject *project = &options->solution.projects[projectIndex];

		// validate the project
		{
			bool8 validProject = true;
			if ( project->name.empty() ) {
				error( "There is a Visual Studio Project that doesn't have a name here.  You need to fill that in.\n" );
				validProject = false;
			}

			// validate each config
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig *config = &project->configs[configIndex];

				if ( config->name.empty() ) {
					error( "There is a config for project \"%s\" that doesn't have a name here.  You need to fill that in.\n", project->name.c_str() );
					validProject = false;
				}

				if ( config->options.name.empty() ) {
					error( "There is a config for project \"%s\" that doesn't have a name set in it's BuildConfig.  You need to fill that in.\n", project->name.c_str() );
					validProject = false;
				}

				if ( config->options.binaryType == BINARY_TYPE_EXE && config->options.binaryFolder.empty() ) {
					error(
						"Build config \"%s\" is an executable, but you never specified an output directory when generating the Visual Studio project \"%s\", config \"%s\".\n"
						"Visual Studio needs this in order to know where to run the executable from when debugging.  You need to set this.\n"
						, config->options.name.c_str(), project->name.c_str(), config->name.c_str()
					);
					validProject = false;
				}
			}

			if ( !validProject ) {
				return false;
			}
		}

		// if the project name has a slash in it then the user wants that project to be in a folder
		// for example a project with the name "projects/games/shooter" means the user wants a project called "shooter" inside a folder called "games", which is in turn inside a folder called "projects"
		// so split the string between slashes, and create project folders for each unique folder name that we find
		{
			String projectNamePath = string_set( project->name.c_str() );
			String fullFolderPath = path_remove_file_from_path( &projectNamePath );

			if ( fullFolderPath.count != projectNamePath.count ) {
				u32 guidIndex = HASHMAP_INVALID_VALUE;
				u32 guidParentIndex = HASHMAP_INVALID_VALUE;

				const char *folderStart = string_cstr( &fullFolderPath );

				while ( *folderStart ) {
					// get the end of the folder
					const char *folderEnd = GetNextSlashInPath( folderStart );

					if ( !folderEnd ) {
						folderEnd = folderStart + strlen( folderStart );
					}

					// make a string from that the start and the end
					u64 folderNameLength = cast( u64, folderEnd ) - cast( u64, folderStart );

					char *folderName = cast( char *, mem_temp_alloc( ( folderNameLength + 1 ) * sizeof( char ) ) );
					strncpy( folderName, folderStart, folderNameLength * sizeof( char ) );
					folderName[folderNameLength] = 0;

					u64 folderNameHash = hash_string( folderName, 0 );

					guidIndex = hashmap_get_value( projectFolderIndices, folderNameHash );

					if ( guidIndex == HASHMAP_INVALID_VALUE ) {
						// not found this folder at this path before so create a GUID for it now
						projectFolders.add( folderName );
						projectGuids.add( CreateVisualStudioGuid() );

						guidIndex = trunc_cast( u32, projectGuids.count - 1 );

						LogVerbose( "%u = %s (parent = %u)\n", guidIndex, folderName, guidParentIndex );

						hashmap_set_value( projectFolderIndices, folderNameHash, guidIndex );

						if ( guidParentIndex != HASHMAP_INVALID_VALUE ) {
							guidParentMappings.add( { guidIndex, guidParentIndex } );
						}
					} else {
						// we have found this folder at this path before
						// if we have other folders/projects to go we will use this index as the parent index
						guidParentIndex = guidIndex;
					}

					folderStart = folderEnd;

					if ( *folderStart == '\\' || *folderStart == '/' ) {
						folderStart++;
					}
				}

				guidParentIndex = guidIndex;

				guidParentMappings.add( { .guidIndex = projectIndex, .guidParentIndex = guidParentIndex } );
			}
		}

		// get all the files that the project will know about
		// the arrays in here get referred to multiple times throughout generating the files for the project
		Array<visualStudioFileFilter_t> sourceFiles;
		sourceFiles.init( mem_get_temp_storage() );
		Array<visualStudioFileFilter_t> headerFiles;
		headerFiles.init( mem_get_temp_storage() );
		Array<visualStudioFileFilter_t> otherFiles;
		otherFiles.init( mem_get_temp_storage() );

		Array<String> filterPaths;
		filterPaths.init( mem_get_temp_storage() );

		// we have ./src/foo/bar.cpp and need both src/foo for folderInFilter and src/foo/bar.cpp for filenameAndPathFromRoot
		{
			auto AddFileUnique = [context, &filterPaths]( Array<visualStudioFileFilter_t>& fileArray, const char* file ) {
				u64 fileIndex = 0;
				for ( ; fileIndex < fileArray.count; ++fileIndex ) {
					if ( string_equals( fileArray[fileIndex].filePath, file ) ) {
						break;
					}
				}

				if ( fileIndex == fileArray.count ) {
					String pathFromBuildConfigToFile = path_relative_path_to( mem_get_temp_storage(), context->inputFilePath.data, file );

					// If we have gone past a drive root then we definitely don't want to include this in the filters in a regular way
					// We could do some extra thing here when there is a lot of ..\\..\\..\\ etc. but for now it's fine
					if ( string_contains( &pathFromBuildConfigToFile, ':' ) ) {
						fileArray.add({ file, String() });
					} else {
						u64 lastSeparator;
						string_find_from_right(&pathFromBuildConfigToFile, PATH_SEPARATOR, &lastSeparator);
						pathFromBuildConfigToFile.count = lastSeparator + 1;
						pathFromBuildConfigToFile.data[lastSeparator] = '\0';

						u64 filterIndex = 0;
						for (; filterIndex < filterPaths.count; ++filterIndex)
						{
							if (string_equals(&filterPaths[filterIndex], &pathFromBuildConfigToFile))
							{
								break;
							}
						}

						if (filterIndex == filterPaths.count)
						{
							filterPaths.add(string_copy(mem_get_temp_storage(), &pathFromBuildConfigToFile));
						}

						fileArray.add({ file, filterPaths[filterIndex] });
					}
				}
			};


			For ( u64, configIndex, 0, project->configs.size() ) {
				For ( u64, sourceFileIndex, 0, project->configs[configIndex].options.sourceFiles.size() ) {
					const char* file = project->configs[configIndex].options.sourceFiles[sourceFileIndex].c_str();
					if ( FileIsSourceFile( file ) ) {
						AddFileUnique( sourceFiles, file );
					} else if ( FileIsHeaderFile(  file ) ) {
						AddFileUnique( headerFiles, file );
					} else {
						AddFileUnique( otherFiles, file );
					}
				}
			}

			For ( u64, fileIndex, 0, project->extraFiles.size() ) {
				const char* file = project->extraFiles[fileIndex].c_str();
				if ( FileIsSourceFile( file ) ) {
					AddFileUnique( sourceFiles, file );
				} else if ( FileIsHeaderFile(  file ) ) {
					AddFileUnique( headerFiles, file );
				} else {
					AddFileUnique( otherFiles, file );
				}
			}
		}

		String projectNameStr = string_set( project->name.c_str() );
		String projectNameNoFolder = path_remove_path_from_file( &projectNameStr );

		// .vcxproj
		{
			u64 tempPos = mem_temp_tell();
			defer { mem_temp_rewind_to( tempPos ); };

			const char *projectPath = temp_printf( "%s%c%s.vcxproj", visualStudioProjectFilesPath, PATH_SEPARATOR, projectNameNoFolder.data );

			printf( "Generating %s ... ", projectPath );

			StringBuilder vcxprojContent = string_builder_create( context->allocator );

			string_builder_appendf( &vcxprojContent, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
			string_builder_appendf( &vcxprojContent, "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

			// generate every single config and platform pairing
			{
				string_builder_appendf( &vcxprojContent, "\t<ItemGroup Label=\"ProjectConfigurations\">\n" );
				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig *config = &project->configs[configIndex];

					For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
						const char *platform = options->solution.platforms[platformIndex].c_str();

						// TODO: Alternative targets
						string_builder_appendf( &vcxprojContent, "\t\t<ProjectConfiguration Include=\"%s|%s\">\n", config->name.c_str(), platform );
						string_builder_appendf( &vcxprojContent, "\t\t\t<Configuration>%s</Configuration>\n", config->name.c_str() );
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
				string_builder_appendf( &vcxprojContent, "\t\t<Keyword>Win32Proj</Keyword>\n" );
				string_builder_appendf( &vcxprojContent, "\t</PropertyGroup>\n" );
			}

			string_builder_appendf( &vcxprojContent, "\t<Import Project=\"$(VCTargetsPath)%cMicrosoft.Cpp.Default.props\" Condition=\"'$(OS)' == 'Windows_NT'\" />\n", PATH_SEPARATOR );

			// for each config and platform, define config type, toolset, out dir, and intermediate dir
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig *config = &project->configs[configIndex];

				const char *fullBinaryName = BuildConfig_GetFullBinaryName( &config->options, mem_get_temp_storage() );

				const char *from = visualStudioProjectFilesPath;
				const char *to = temp_printf( "%s%c%s", context->inputFilePath.data, PATH_SEPARATOR, fullBinaryName );

				String pathFromSolutionToBinary = path_relative_path_to( mem_get_temp_storage(), from, to );
				pathFromSolutionToBinary = path_remove_file_from_path( &pathFromSolutionToBinary );

				For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
					const char *platform = options->solution.platforms[platformIndex].c_str();

					string_builder_appendf( &vcxprojContent, "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\" Label=\"Configuration\">\n", config->name.c_str(), platform );
					string_builder_appendf( &vcxprojContent, "\t\t<ConfigurationType>Makefile</ConfigurationType>\n" );
					string_builder_appendf( &vcxprojContent, "\t\t<UseDebugLibraries>false</UseDebugLibraries>\n" );
					string_builder_appendf( &vcxprojContent, "\t\t<PlatformToolset>v143</PlatformToolset>\n" );
					string_builder_appendf( &vcxprojContent, "\t\t<OutDir>%s</OutDir>\n", string_cstr( &pathFromSolutionToBinary ) );
					string_builder_appendf( &vcxprojContent, "\t\t<IntDir>%s%cintermediate</IntDir>\n", string_cstr( &pathFromSolutionToBinary ), PATH_SEPARATOR );
					string_builder_appendf( &vcxprojContent, "\t</PropertyGroup>\n" );
				}
			}

			string_builder_appendf( &vcxprojContent, "\t<Import Project=\"$(VCTargetsPath)%cMicrosoft.Cpp.props\" Condition=\"'$(OS)' == 'Windows_NT'\" />\n", PATH_SEPARATOR );

			// not sure what this is or why we need this one but visual studio seems to want it
			string_builder_appendf( &vcxprojContent, "\t<ImportGroup Label=\"ExtensionSettings\">\n" );
			string_builder_appendf( &vcxprojContent, "\t</ImportGroup>\n" );

			// for each config and platform, import the property sheets that visual studio requires
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig *config = &project->configs[configIndex];

				For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
					const char *platform = options->solution.platforms[platformIndex].c_str();

					string_builder_appendf( &vcxprojContent, "\t<ImportGroup Label=\"PropertySheets\" Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\">\n", config->name.c_str(), platform );
					string_builder_appendf( &vcxprojContent, "\t\t<Import Project=\"$(UserRootDir)%cMicrosoft.Cpp.$(Platform).user.props\" Condition=\"exists(\'$(UserRootDir)%cMicrosoft.Cpp.$(Platform).user.props\')\" Label=\"LocalAppDataPlatform\" />\n", PATH_SEPARATOR, PATH_SEPARATOR );
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
				VisualStudioConfig *config = &project->configs[configIndex];

				For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
					const char *platform = options->solution.platforms[platformIndex].c_str();

					string_builder_appendf( &vcxprojContent, "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\">\n", config->name.c_str(), platform );

					// external include paths
					string_builder_appendf( &vcxprojContent, "\t\t<ExternalIncludePath>" );
					For ( u64, includePathIndex, 0, config->options.additionalIncludes.size() ) {
						const char *additionalInclude = config->options.additionalIncludes[includePathIndex].c_str();

						if ( path_is_absolute( additionalInclude ) ) {
							string_builder_appendf( &vcxprojContent, "%s;", config->options.additionalIncludes[includePathIndex].c_str() );
						} else {
							string_builder_appendf( &vcxprojContent, "%s%c%s;", pathFromSolutionToInputFile, PATH_SEPARATOR, config->options.additionalIncludes[includePathIndex].c_str() );
						}
					}
					string_builder_appendf( &vcxprojContent, "$(ExternalIncludePath)" );
					string_builder_appendf( &vcxprojContent, "</ExternalIncludePath>\n" );

					// external library paths
					string_builder_appendf( &vcxprojContent, "\t\t<LibraryPath>" );
					For ( u64, libPathIndex, 0, config->options.additionalLibPaths.size() ) {
						const char *additionalLibPath = config->options.additionalLibPaths[libPathIndex].c_str();

						if ( path_is_absolute( additionalLibPath ) ) {
							string_builder_appendf( &vcxprojContent, "%s;", additionalLibPath );
						} else {
							string_builder_appendf( &vcxprojContent, "%s%c%s;", pathFromSolutionToInputFile, PATH_SEPARATOR, additionalLibPath );
						}
					}
					string_builder_appendf( &vcxprojContent, "$(LibraryPath)" );
					string_builder_appendf( &vcxprojContent, "</LibraryPath>\n" );

					// output path
					string_builder_appendf( &vcxprojContent, "\t\t<NMakeOutput>%s</NMakeOutput>\n", config->options.binaryFolder.c_str() );

					const char *fullConfigName = config->options.name.c_str();

					String inputFileNoPathStr = string_set( context->inputFile );
					String inputFileNoPath = path_remove_path_from_file( &inputFileNoPathStr );

					String inputFileRelative = path_join( mem_get_temp_storage(), pathFromSolutionToInputFile, inputFileNoPath.data );

					// const char *appPath = path_remove_file_from_path( path_app_path() );
					String appPath = path_app_path( mem_get_temp_storage() );
					appPath = path_remove_file_from_path( &appPath );
					const char *appPathCStr = string_cstr( &appPath );

					// build command
					string_builder_appendf( &vcxprojContent, "\t\t<NMakeBuildCommandLine>\"%s%c%s\" %s %s%s %s", appPathCStr, PATH_SEPARATOR, BUILDER_PROGRAM_NAME, inputFileRelative.data, ARG_CONFIG, fullConfigName, ARG_VISUAL_STUDIO_BUILD );
					For ( u32, argIndex, 0, config->additionalBuildArgs.size() ) {
						string_builder_appendf( &vcxprojContent, " %s", config->additionalBuildArgs[argIndex].c_str() );
					}
					string_builder_appendf( &vcxprojContent, "</NMakeBuildCommandLine>\n" );

					// rebuild command
					string_builder_appendf( &vcxprojContent, "\t\t<NMakeReBuildCommandLine>\"%s%c%s\" %s %s%s %s", appPathCStr, PATH_SEPARATOR, BUILDER_PROGRAM_NAME, inputFileRelative.data, ARG_CONFIG, fullConfigName, ARG_VISUAL_STUDIO_BUILD );
					For ( u32, argIndex, 0, config->additionalBuildArgs.size() ) {
						string_builder_appendf( &vcxprojContent, " %s", config->additionalBuildArgs[argIndex].c_str() );
					}
					string_builder_appendf( &vcxprojContent, "</NMakeReBuildCommandLine>\n" );

					// clean comand
					string_builder_appendf( &vcxprojContent, "\t\t<NMakeCleanCommandLine>\"%s%c%s\" %s %s</NMakeCleanCommandLine>\n", appPathCStr, PATH_SEPARATOR, BUILDER_PROGRAM_NAME, ARG_NUKE, config->options.binaryFolder.c_str() );

					// preprocessor definitions
					string_builder_appendf( &vcxprojContent, "\t\t<NMakePreprocessorDefinitions>" );
					For ( u64, definitionIndex, 0, config->options.defines.size() ) {
						string_builder_appendf( &vcxprojContent, "%s;", config->options.defines[definitionIndex].c_str() );
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
				auto WriteFilterFilesToVcxproj = []( StringBuilder *stringBuilder, const Array<visualStudioFileFilter_t>& files, const char* tag) {
					if ( files.count == 0 ) {
						return;
					}

					string_builder_appendf( stringBuilder, "\t<ItemGroup>\n" );

					For ( u64, fileIndex, 0, files.count ) {
						string_builder_appendf( stringBuilder, "\t\t<%s Include=\"%s\" />\n", tag, files[fileIndex].filePath );
					}

					string_builder_appendf( stringBuilder, "\t</ItemGroup>\n" );
				};

				WriteFilterFilesToVcxproj( &vcxprojContent, sourceFiles, "ClCompile" );
				WriteFilterFilesToVcxproj( &vcxprojContent, headerFiles, "ClInclude" );
				WriteFilterFilesToVcxproj( &vcxprojContent, otherFiles, "None" );
			}

			string_builder_appendf( &vcxprojContent, "\t<Import Project=\"$(VCTargetsPath)%cMicrosoft.Cpp.targets\" Condition=\"'$(OS)' == 'Windows_NT'\" />\n", PATH_SEPARATOR );

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
			u64 tempPos = mem_temp_tell();
			defer { mem_temp_rewind_to( tempPos ); };

			const char *projectPath = temp_printf( "%s%c%s.vcxproj.user", visualStudioProjectFilesPath, PATH_SEPARATOR, projectNameNoFolder.data );

			printf( "Generating %s ... ", projectPath );

			StringBuilder vcxprojContent = string_builder_create( context->allocator );

			string_builder_appendf( &vcxprojContent, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
			string_builder_appendf( &vcxprojContent, "<Project ToolsVersion=\"Current\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

			string_builder_appendf( &vcxprojContent, "\t<PropertyGroup>\n" );
			string_builder_appendf( &vcxprojContent, "\t\t<ShowAllFiles>false</ShowAllFiles>\n" );
			string_builder_appendf( &vcxprojContent, "\t</PropertyGroup>\n" );

			// for each config and platform, generate the debugger settings
			{
				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig *config = &project->configs[configIndex];

					const char *fullBinaryName = BuildConfig_GetFullBinaryName( &config->options, mem_get_temp_storage() );

					const char *from = visualStudioProjectFilesPath;
					const char *to = temp_printf( "%s%c%s", context->inputFilePath.data, PATH_SEPARATOR, fullBinaryName );
					//to = path_canonicalise( to );

					String pathFromSolutionToBinaryStr = path_relative_path_to( mem_get_temp_storage(), from, to );

					For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
						const char *platform = options->solution.platforms[platformIndex].c_str();

						string_builder_appendf( &vcxprojContent, "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\">\n", config->name.c_str(), platform );
						string_builder_appendf( &vcxprojContent, "\t\t<DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>\n" );	// TODO(DM): do want to include the other debugger types?
						string_builder_appendf( &vcxprojContent, "\t\t<LocalDebuggerDebuggerType>Auto</LocalDebuggerDebuggerType>\n" );
						string_builder_appendf( &vcxprojContent, "\t\t<LocalDebuggerAttach>false</LocalDebuggerAttach>\n" );
						string_builder_appendf( &vcxprojContent, "\t\t<LocalDebuggerCommand>%s</LocalDebuggerCommand>\n",  string_cstr( &pathFromSolutionToBinaryStr ) );
						string_builder_appendf( &vcxprojContent, "\t\t<LocalDebuggerWorkingDirectory>%s</LocalDebuggerWorkingDirectory>\n", config->runFromDirectory.c_str() );

						// if debugger arguments were specified, put those in
						if ( config->debuggerArguments.size() > 0 ) {
							string_builder_appendf( &vcxprojContent, "\t\t<LocalDebuggerCommandArguments>\n" );
							For ( u64, argIndex, 0, config->debuggerArguments.size() ) {
								string_builder_appendf( &vcxprojContent, "%s ", config->debuggerArguments[argIndex].c_str() );
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
			u64 tempPos = mem_temp_tell();
			defer { mem_temp_rewind_to( tempPos ); };

			const char *projectPath = temp_printf( "%s%c%s.vcxproj.filters", visualStudioProjectFilesPath, PATH_SEPARATOR, projectNameNoFolder.data );

			printf( "Generating %s ... ", projectPath );

			StringBuilder vcxprojContent = string_builder_create( context->allocator );

			string_builder_appendf( &vcxprojContent, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
			string_builder_appendf( &vcxprojContent, "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

			string_builder_appendf( &vcxprojContent, "\t<ItemGroup>\n" );

			// write all filter guids
			For ( u64, filterPathIndex, 0, filterPaths.count ) {
				String filterPath = path_fix_slashes( mem_get_temp_storage(), &filterPaths[filterPathIndex]);

				const char *guid = CreateVisualStudioGuid();

				string_builder_appendf( &vcxprojContent, "\t\t<Filter Include=\"%s\">\n", filterPath.data );
				string_builder_appendf( &vcxprojContent, "\t\t\t<UniqueIdentifier>{%s}</UniqueIdentifier>\n", guid );
				string_builder_appendf( &vcxprojContent, "\t\t</Filter>\n" );
			}

			string_builder_appendf( &vcxprojContent, "\t</ItemGroup>\n" );

			// now put all files in the filter
			// visual studio requires that we list each file by type
			{
				auto WriteFileFilters = []( StringBuilder *stringBuilder, const Array<visualStudioFileFilter_t> &fileFilters, const char *tag ) {
					if ( fileFilters.count == 0 ) {
						return;
					}

					string_builder_appendf( stringBuilder, "\t<ItemGroup>\n" );

					For ( u64, fileIndex, 0, fileFilters.count ) {
						const visualStudioFileFilter_t* file = &fileFilters[fileIndex];

						if ( file->fileFilter.data == NULL ) {
							string_builder_appendf( stringBuilder, "\t\t<%s Include=\"%s\" />\n", tag, file->filePath );
						} else {
							string_builder_appendf( stringBuilder, "\t\t<%s Include=\"%s\">\n", tag, file->filePath );
							string_builder_appendf( stringBuilder, "\t\t\t<Filter>%s</Filter>\n", file->fileFilter.data );
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
		u64 tempPos = mem_temp_tell();
		defer { mem_temp_rewind_to( tempPos ); };

		printf( "Generating %s ... ", solutionFilename );

		StringBuilder slnContent = string_builder_create( context->allocator );

		string_builder_appendf( &slnContent, "\n" );
		string_builder_appendf( &slnContent, "Microsoft Visual Studio Solution File, Format Version 12.00\n" );
		string_builder_appendf( &slnContent, "# Visual Studio Version 17\n" );
		string_builder_appendf( &slnContent, "VisualStudioVersion = 17.7.34202.233\n" );		// TODO(DM): how do we query windows for this?
		string_builder_appendf( &slnContent, "MinimunVisualStudioVersion = 10.0.40219.1\n" );	// TODO(DM): how do we query windows for this?

		// project GUIDs
		For ( u64, projectIndex, 0, options->solution.projects.size() ) {
			VisualStudioProject *project = &options->solution.projects[projectIndex];

			// const char *projectName = path_remove_path_from_file( project->name.c_str() );
			String projectNameStr = string_set( project->name.c_str() );
			String projectName = path_remove_path_from_file( &projectNameStr );

			string_builder_appendf( &slnContent, "Project(\"{%s}\") = \"%s\", \"%s.vcxproj\", \"{%s}\"\n", VISUAL_STUDIO_CPP_PROJECT_TYPE_GUID, projectName.data, projectName.data, projectGuids[projectIndex] );
			string_builder_appendf( &slnContent, "EndProject\n" );
		}

		// project folder GUIDs
		For ( u64, projectFolderIndex, 0, projectFolders.count ) {
			const char *folderName = projectFolders[projectFolderIndex];

			u64 folderGuidIndex = options->solution.projects.size() + projectFolderIndex;

			string_builder_appendf( &slnContent, "Project(\"{%s}\") = \"%s\", \"%s\", \"{%s}\"\n", VISUAL_STUDIO_FOLDER_PROJECT_TYPE_GUID, folderName, folderName, projectGuids[folderGuidIndex] );
			string_builder_appendf( &slnContent, "EndProject\n" );
		}

		string_builder_appendf( &slnContent, "Global\n" );
		{
			// which config|platform maps to which config|platform?
			string_builder_appendf( &slnContent, "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n" );
			For ( u64, projectIndex, 0, options->solution.projects.size() ) {
				VisualStudioProject *project = &options->solution.projects[projectIndex];

				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig *config = &project->configs[configIndex];

					For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
						const char *platform = options->solution.platforms[platformIndex].c_str();

						string_builder_appendf( &slnContent, "\t\t%s|%s = %s|%s\n", config->name.c_str(), platform, config->name.c_str(), platform );
					}
				}
			}
			string_builder_appendf( &slnContent, "\tEndGlobalSection\n" );

			// which project config|platform is active?
			string_builder_appendf( &slnContent, "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n" );
			For ( u64, projectIndex, 0, options->solution.projects.size() ) {
				VisualStudioProject *project = &options->solution.projects[projectIndex];

				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig *config = &project->configs[configIndex];

					For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
						const char *platform = options->solution.platforms[platformIndex].c_str();

						const char *projectGuid = projectGuids[projectIndex];

						// TODO: the first config and platform in this line are actually the ones that the PROJECT has, not the SOLUTION
						// but we dont use those, and we should
						string_builder_appendf( &slnContent, "\t\t{%s}.%s|%s.ActiveCfg = %s|%s\n", projectGuid, config->name.c_str(), platform, config->name.c_str(), platform );
						string_builder_appendf( &slnContent, "\t\t{%s}.%s|%s.Build.0 = %s|%s\n", projectGuid, config->name.c_str(), platform, config->name.c_str(), platform );
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
			if ( guidParentMappings.count > 0 ) {
				string_builder_appendf( &slnContent, "\tGlobalSection(NestedProjects) = preSolution\n" );
				For ( u64, projectIndex, 0, guidParentMappings.count ) {
					guidParentMapping_t *mapping = &guidParentMappings[projectIndex];

					string_builder_appendf( &slnContent, "\t\t{%s} = {%s}\n", projectGuids[mapping->guidIndex], projectGuids[mapping->guidParentIndex] );
				}
				string_builder_appendf( &slnContent, "\tEndGlobalSection\n" );
			}

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

		printf( "Done\n" );
	}

	printf( "\n" );

	return true;
}

#pragma clang diagnostic pop
