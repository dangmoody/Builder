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

#include "int_types.h"
#include "linear_allocator.h"
#include "string_builder.h"
#include "string.h"
#include "array.inl"
#include "file.h"
#include "paths.h"
#include "typecast.h"
#include "temp_storage.h"
#include "hash.h"
#include "hashmap.h"
#include "debug.h"
#include "defer.h"

#if defined( _WIN32 )
#include <Shlwapi.h>
#elif defined( __linux__ )
#include <uuid/uuid.h>
#endif


// some project type guids are pre-determined by visual studio
#define VISUAL_STUDIO_CPP_PROJECT_TYPE_GUID		"8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942"	// c++ project
#define VISUAL_STUDIO_FOLDER_PROJECT_TYPE_GUID	"2150E333-8FDC-42A3-9474-1A3956D46DE8"	// project folder

struct visualStudioFileFilter_t {
	const char*	filePath;
	string_t	fileFilter;
};

// data layout comes from: https://learn.microsoft.com/en-us/windows/win32/api/guiddef/ns-guiddef-guid
// DM: I don't see a good enough argument that this is common enough that we want this in Core, currently, so I'm leaving this here for now
static const char *CreateVisualStudioGuid() {
#if defined( _WIN32 )
	GUID guid;
	HRESULT hr = CoCreateGuid( &guid );
	Assert( hr == S_OK );

	return TempPrintf( "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] );
#elif defined( __linux__ )
	const u64 guidStringLength = 37;
	char *guidString = Cast( char *, Mem_TempAlloc( guidStringLength * sizeof( char ) ) );

	uuid_t uuid;
	uuid_generate( uuid );
	uuid_unparse_upper( uuid, guidString );

	return guidString;
#endif
}

struct visualStudioNukeContext_t {
	array_t<const char *>	fileExtensionsToCheck;
	string_t				dotVSFolder;
};

static void VS_DeleteOldProjectFilesCallback( const fileInfo_t *fileInfo, void *userData ) {
	visualStudioNukeContext_t *nukeContext = Cast( visualStudioNukeContext_t *, userData );

	if ( fileInfo->isDirectory && String_Equals( fileInfo->filename, ".vs" ) ) {
		// TODO: DM: 09/05/2026: not sure we want to store this in temp storage
		nukeContext->dotVSFolder = String_Alloc( Mem_GetTempStorage(), fileInfo->fullFilename, strlen( fileInfo->fullFilename ) + 1 );
	}

	For ( u32, fileExtensionIndex, 0, nukeContext->fileExtensionsToCheck.count ) {
		if ( String_EndsWith( fileInfo->fullFilename, nukeContext->fileExtensionsToCheck[fileExtensionIndex] ) ) {
			LogVerbose( "Deleting file \"%s\"\n", fileInfo->fullFilename );

			if ( FS_DeleteFile( fileInfo->fullFilename ) ) {
				break;
			} else {
				Warning( "Failed to delete old Visual Studio file \"%s\" while deleting old Visual Studio files.  You will have to delete this one yourself.  Sorry.\n", fileInfo->fullFilename );
			}
		}
	}
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"

bool8 GenerateVisualStudioSolution( buildContext_t *context, BuilderOptions *options ) {
	Assert( context );
	Assert( context->inputFile );
	Assert( context->inputFilePath.data );
	Assert( options );

	array_t<char *> projectFolders;
	projectFolders.Init( Mem_GetTempStorage() );

	hashmap_t *projectFolderIndices = HM_Create( Mem_GetTempStorage(), 1 );

	struct guidParentMapping_t {
		u64	guidIndex;
		u64	guidParentIndex;
	};

	array_t<guidParentMapping_t> guidParentMappings;
	guidParentMappings.Init( Mem_GetTempStorage() );

	// validate the solution
	{
		bool8 validSolution = true;
		if ( options->solution.name.empty() ) {
			Error( "You never set the name of the solution.  I need that.\n" );
			validSolution = false;
		}

		if ( options->solution.platforms.size() < 1 ) {
			Error( "You must set at least one platform when generating a Visual Studio Solution.\n" );
			validSolution = false;
		}

		if ( options->solution.projects.size() < 1 ) {
			Error( "As well as a Solution, you must also generate at least one Visual Studio Project to go with it.\n" );
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

				For ( u64, defaultPlatformIndex, 0, COUNT_OF( defaultPlatformNames ) ) {
					const char *defaultPlatformName = defaultPlatformNames[defaultPlatformIndex];

					if ( String_Equals( platform, defaultPlatformName ) ) {
						foundDefaultPlatformName = true;
						break;
					}
				}
			}

			if ( !foundDefaultPlatformName ) {
				// u64 pos = Mem_TempTell();
				// defer { Mem_TempRewindTo( pos ); };

				stringBuilder_t error = SB_Create( context->allocator );
				SB_Appendf( &error, "None of your platform names are any of the Visual Studio recognized defaults:\n" );
				For ( u64, platformIndex, 0, COUNT_OF( defaultPlatformNames ) ) {
					SB_Appendf( &error, "\t- %s\n", defaultPlatformNames[platformIndex] );
				}
				SB_Appendf( &error, "Visual Studio relies on those specific names in order to generate fields like \"Executable Path\" properly (for example).\n" );
				SB_Appendf( &error, "Builder will still generate the solution, but know that not setting at least one platform name to any of these defaults will cause certain fields in the property pages of your Visual Studio project to not be correct.\n" );
				SB_Appendf( &error, "You have been warned.\n" );

				Warning( SB_ToString( &error ) );
			}
		}
	}

	const char *visualStudioProjectFilesPath = NULL;
	if ( !options->solution.path.empty() ) {
		visualStudioProjectFilesPath = TempPrintf( "%s%c%s", context->inputFilePath.data, PATH_SEPARATOR, options->solution.path.c_str() );
	} else {
		visualStudioProjectFilesPath = context->inputFilePath.data;
	}
	//visualStudioProjectFilesPath = path_canonicalise( visualStudioProjectFilesPath );

	// delete old VS files if they exist
	// but keep the root because we're about to re-populate it
	{
		visualStudioNukeContext_t nukeContext = {};
		nukeContext.fileExtensionsToCheck.Init( Mem_GetTempStorage() );
		nukeContext.fileExtensionsToCheck.Add( ".sln" );
		nukeContext.fileExtensionsToCheck.Add( ".vcxproj" );
		nukeContext.fileExtensionsToCheck.Add( ".vcxproj.user" );
		nukeContext.fileExtensionsToCheck.Add( ".vcxproj.filters" );

		FS_GetAllFilesInFolder( visualStudioProjectFilesPath, FILE_VISIT_FILES | FILE_VISIT_FOLDERS, VS_DeleteOldProjectFilesCallback, &nukeContext );

		if ( nukeContext.dotVSFolder.data ) {
			NukeFolder( nukeContext.dotVSFolder.data, true, g_verbose );
		}
	}

	const char *solutionFilename = TempPrintf( "%s%c%s.sln", visualStudioProjectFilesPath, PATH_SEPARATOR, options->solution.name.c_str() );

	// get relative path from visual studio to the input file
	string_t pathFromSolutionToInputFileStr = Path_RelativePathTo( context->allocator, visualStudioProjectFilesPath, context->inputFilePath.data );
	const char *pathFromSolutionToInputFile = pathFromSolutionToInputFileStr.data;
	Assert( pathFromSolutionToInputFile != NULL && !String_Equals( pathFromSolutionToInputFile, "" ) );

	// give each project a guid
	array_t<const char *> projectGuids;
	projectGuids.Init( Mem_GetTempStorage() );
	projectGuids.Resize( options->solution.projects.size() );

	For ( u64, guidIndex, 0, projectGuids.count ) {
		projectGuids[guidIndex] = CreateVisualStudioGuid();
	}

	if ( !FS_CreateFolderIfItDoesntExist( visualStudioProjectFilesPath ) ) {
		s32 errorCode = GetLastErrorCode();
		Error( "Failed to create the Visual Studio Solution folder.  Error code: " ERROR_CODE_FORMAT "\n", errorCode );

		return false;
	}

	// generate each .vcxproj
	For ( u64, projectIndex, 0, options->solution.projects.size() ) {
		VisualStudioProject *project = &options->solution.projects[projectIndex];

		// validate the project
		{
			bool8 validProject = true;
			if ( project->name.empty() ) {
				Error( "There is a Visual Studio Project that doesn't have a name here.  You need to fill that in.\n" );
				validProject = false;
			}

			// validate each config
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig *config = &project->configs[configIndex];

				if ( config->name.empty() ) {
					Error( "There is a config for project \"%s\" that doesn't have a name here.  You need to fill that in.\n", project->name.c_str() );
					validProject = false;
				}

				if ( config->options.name.empty() ) {
					Error( "There is a config for project \"%s\" that doesn't have a name set in it's BuildConfig.  You need to fill that in.\n", project->name.c_str() );
					validProject = false;
				}

				if ( config->options.binaryType == BINARY_TYPE_EXE && config->options.binaryFolder.empty() ) {
					Error(
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
			string_t projectNamePath = String_Set( project->name.c_str() );
			string_t fullFolderPath = Path_RemoveFileFromPath( &projectNamePath );

			if ( fullFolderPath.count != projectNamePath.count ) {
				u32 guidIndex = HASHMAP_INVALID_VALUE;
				u32 guidParentIndex = HASHMAP_INVALID_VALUE;

				const char *folderStart = String_Cstr( &fullFolderPath );

				while ( *folderStart ) {
					// get the end of the folder
					const char *folderEnd = GetNextSlashInPath( folderStart );

					if ( !folderEnd ) {
						folderEnd = folderStart + strlen( folderStart );
					}

					// make a string from that the start and the end
					u64 folderNameLength = Cast( u64, folderEnd ) - Cast( u64, folderStart );

					char *folderName = Cast( char *, Mem_TempAlloc( ( folderNameLength + 1 ) * sizeof( char ) ) );
					strncpy( folderName, folderStart, folderNameLength * sizeof( char ) );
					folderName[folderNameLength] = 0;

					u64 folderNameHash = HashString( folderName, 0 );

					guidIndex = HM_GetValue( projectFolderIndices, folderNameHash );

					if ( guidIndex == HASHMAP_INVALID_VALUE ) {
						// not found this folder at this path before so create a GUID for it now
						projectFolders.Add( folderName );
						projectGuids.Add( CreateVisualStudioGuid() );

						guidIndex = TruncCast( u32, projectGuids.count - 1 );

						LogVerbose( "%u = %s (parent = %u)\n", guidIndex, folderName, guidParentIndex );

						HM_SetValue( projectFolderIndices, folderNameHash, guidIndex );

						if ( guidParentIndex != HASHMAP_INVALID_VALUE ) {
							guidParentMappings.Add( { guidIndex, guidParentIndex } );
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

				guidParentMappings.Add( { .guidIndex = projectIndex, .guidParentIndex = guidParentIndex } );
			}
		}

		// get all the files that the project will know about
		// the arrays in here get referred to multiple times throughout generating the files for the project
		array_t<visualStudioFileFilter_t> sourceFiles;
		sourceFiles.Init( Mem_GetTempStorage() );
		array_t<visualStudioFileFilter_t> headerFiles;
		headerFiles.Init( Mem_GetTempStorage() );
		array_t<visualStudioFileFilter_t> otherFiles;
		otherFiles.Init( Mem_GetTempStorage() );

		array_t<string_t> filterPaths;
		filterPaths.Init( Mem_GetTempStorage() );

		// we have ./src/foo/bar.cpp and need both src/foo for folderInFilter and src/foo/bar.cpp for filenameAndPathFromRoot
		{
			auto AddFileUnique = [context, &filterPaths]( array_t<visualStudioFileFilter_t>& fileArray, const char* file ) {
				u64 fileIndex = 0;
				for ( ; fileIndex < fileArray.count; ++fileIndex ) {
					if ( String_Equals( fileArray[fileIndex].filePath, file ) ) {
						break;
					}
				}

				if ( fileIndex == fileArray.count ) {
					string_t pathFromBuildConfigToFile = Path_RelativePathTo( Mem_GetTempStorage(), context->inputFilePath.data, file );

					// If we have gone past a drive root then we definitely don't want to include this in the filters in a regular way
					// We could do some extra thing here when there is a lot of ..\\..\\..\\ etc. but for now it's fine
					if ( String_Contains( &pathFromBuildConfigToFile, ':' ) ) {
						fileArray.Add({ file, string_t() });
					} else {
						u64 lastSeparator;
						String_FindFromRight(&pathFromBuildConfigToFile, PATH_SEPARATOR, &lastSeparator);
						pathFromBuildConfigToFile.count = lastSeparator + 1;
						pathFromBuildConfigToFile.data[lastSeparator] = '\0';

						u64 filterIndex = 0;
						for (; filterIndex < filterPaths.count; ++filterIndex)
						{
							if (String_Equals(&filterPaths[filterIndex], &pathFromBuildConfigToFile))
							{
								break;
							}
						}

						if (filterIndex == filterPaths.count)
						{
							filterPaths.Add(String_Copy(Mem_GetTempStorage(), &pathFromBuildConfigToFile));
						}

						fileArray.Add({ file, filterPaths[filterIndex] });
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

		string_t projectNameStr = String_Set( project->name.c_str() );
		string_t projectNameNoFolder = Path_RemovePathFromFile( &projectNameStr );

		// .vcxproj
		{
			u64 tempPos = Mem_TempTell();
			defer { Mem_TempRewindTo( tempPos ); };

			const char *projectPath = TempPrintf( "%s%c%s.vcxproj", visualStudioProjectFilesPath, PATH_SEPARATOR, projectNameNoFolder.data );

			printf( "Generating %s ... ", projectPath );

			stringBuilder_t vcxprojContent = SB_Create( context->allocator );

			SB_Appendf( &vcxprojContent, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
			SB_Appendf( &vcxprojContent, "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

			// generate every single config and platform pairing
			{
				SB_Appendf( &vcxprojContent, "\t<ItemGroup Label=\"ProjectConfigurations\">\n" );
				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig *config = &project->configs[configIndex];

					For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
						const char *platform = options->solution.platforms[platformIndex].c_str();

						// TODO: Alternative targets
						SB_Appendf( &vcxprojContent, "\t\t<ProjectConfiguration Include=\"%s|%s\">\n", config->name.c_str(), platform );
						SB_Appendf( &vcxprojContent, "\t\t\t<Configuration>%s</Configuration>\n", config->name.c_str() );
						SB_Appendf( &vcxprojContent, "\t\t\t<Platform>%s</Platform>\n", platform );
						SB_Appendf( &vcxprojContent, "\t\t</ProjectConfiguration>\n" );
					}
				}
				SB_Appendf( &vcxprojContent, "\t</ItemGroup>\n" );
			}

			// project globals
			{
				SB_Appendf( &vcxprojContent, "\t<PropertyGroup Label=\"Globals\">\n" );
				SB_Appendf( &vcxprojContent, "\t\t<VCProjectVersion>17.0</VCProjectVersion>\n" );
				SB_Appendf( &vcxprojContent, "\t\t<ProjectGuid>{%s}</ProjectGuid>\n", projectGuids[projectIndex] );
				SB_Appendf( &vcxprojContent, "\t\t<IgnoreWarnCompileDuplicatedFilename>true</IgnoreWarnCompileDuplicatedFilename>\n" );
				SB_Appendf( &vcxprojContent, "\t\t<Keyword>Win32Proj</Keyword>\n" );
				SB_Appendf( &vcxprojContent, "\t</PropertyGroup>\n" );
			}

			SB_Appendf( &vcxprojContent, "\t<Import Project=\"$(VCTargetsPath)%cMicrosoft.Cpp.Default.props\" Condition=\"'$(OS)' == 'Windows_NT'\" />\n", PATH_SEPARATOR );

			// for each config and platform, define config type, toolset, out dir, and intermediate dir
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig *config = &project->configs[configIndex];

				const char *fullBinaryName = BuildConfig_GetFullBinaryName( &config->options, Mem_GetTempStorage() );

				const char *from = visualStudioProjectFilesPath;
				const char *to = TempPrintf( "%s%c%s", context->inputFilePath.data, PATH_SEPARATOR, fullBinaryName );

				string_t pathFromSolutionToBinary = Path_RelativePathTo( Mem_GetTempStorage(), from, to );
				pathFromSolutionToBinary = Path_RemoveFileFromPath( &pathFromSolutionToBinary );

				For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
					const char *platform = options->solution.platforms[platformIndex].c_str();

					SB_Appendf( &vcxprojContent, "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\" Label=\"Configuration\">\n", config->name.c_str(), platform );
					SB_Appendf( &vcxprojContent, "\t\t<ConfigurationType>Makefile</ConfigurationType>\n" );
					SB_Appendf( &vcxprojContent, "\t\t<UseDebugLibraries>false</UseDebugLibraries>\n" );
					SB_Appendf( &vcxprojContent, "\t\t<PlatformToolset>v143</PlatformToolset>\n" );
					SB_Appendf( &vcxprojContent, "\t\t<OutDir>%s</OutDir>\n", String_Cstr( &pathFromSolutionToBinary ) );
					SB_Appendf( &vcxprojContent, "\t\t<IntDir>%s%cintermediate</IntDir>\n", String_Cstr( &pathFromSolutionToBinary ), PATH_SEPARATOR );
					SB_Appendf( &vcxprojContent, "\t</PropertyGroup>\n" );
				}
			}

			SB_Appendf( &vcxprojContent, "\t<Import Project=\"$(VCTargetsPath)%cMicrosoft.Cpp.props\" Condition=\"'$(OS)' == 'Windows_NT'\" />\n", PATH_SEPARATOR );

			// not sure what this is or why we need this one but visual studio seems to want it
			SB_Appendf( &vcxprojContent, "\t<ImportGroup Label=\"ExtensionSettings\">\n" );
			SB_Appendf( &vcxprojContent, "\t</ImportGroup>\n" );

			// for each config and platform, import the property sheets that visual studio requires
			For ( u64, configIndex, 0, project->configs.size() ) {
				VisualStudioConfig *config = &project->configs[configIndex];

				For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
					const char *platform = options->solution.platforms[platformIndex].c_str();

					SB_Appendf( &vcxprojContent, "\t<ImportGroup Label=\"PropertySheets\" Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\">\n", config->name.c_str(), platform );
					SB_Appendf( &vcxprojContent, "\t\t<Import Project=\"$(UserRootDir)%cMicrosoft.Cpp.$(Platform).user.props\" Condition=\"exists(\'$(UserRootDir)%cMicrosoft.Cpp.$(Platform).user.props\')\" Label=\"LocalAppDataPlatform\" />\n", PATH_SEPARATOR, PATH_SEPARATOR );
					SB_Appendf( &vcxprojContent, "\t</ImportGroup>\n" );
				}
			}

			// not sure what this is or why we need this one but visual studio seems to want it
			SB_Appendf( &vcxprojContent, "\t<PropertyGroup Label=\"UserMacros\" />\n" );

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

					SB_Appendf( &vcxprojContent, "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\">\n", config->name.c_str(), platform );

					// external include paths
					SB_Appendf( &vcxprojContent, "\t\t<ExternalIncludePath>" );
					For ( u64, includePathIndex, 0, config->options.additionalIncludes.size() ) {
						const char *additionalInclude = config->options.additionalIncludes[includePathIndex].c_str();

						if ( Path_IsAbsolute( additionalInclude ) ) {
							SB_Appendf( &vcxprojContent, "%s;", config->options.additionalIncludes[includePathIndex].c_str() );
						} else {
							SB_Appendf( &vcxprojContent, "%s%c%s;", pathFromSolutionToInputFile, PATH_SEPARATOR, config->options.additionalIncludes[includePathIndex].c_str() );
						}
					}
					SB_Appendf( &vcxprojContent, "$(ExternalIncludePath)" );
					SB_Appendf( &vcxprojContent, "</ExternalIncludePath>\n" );

					// external library paths
					SB_Appendf( &vcxprojContent, "\t\t<LibraryPath>" );
					For ( u64, libPathIndex, 0, config->options.additionalLibPaths.size() ) {
						const char *additionalLibPath = config->options.additionalLibPaths[libPathIndex].c_str();

						if ( Path_IsAbsolute( additionalLibPath ) ) {
							SB_Appendf( &vcxprojContent, "%s;", additionalLibPath );
						} else {
							SB_Appendf( &vcxprojContent, "%s%c%s;", pathFromSolutionToInputFile, PATH_SEPARATOR, additionalLibPath );
						}
					}
					SB_Appendf( &vcxprojContent, "$(LibraryPath)" );
					SB_Appendf( &vcxprojContent, "</LibraryPath>\n" );

					// output path
					SB_Appendf( &vcxprojContent, "\t\t<NMakeOutput>%s</NMakeOutput>\n", config->options.binaryFolder.c_str() );

					const char *fullConfigName = config->options.name.c_str();

					string_t inputFileNoPathStr = String_Set( context->inputFile );
					string_t inputFileNoPath = Path_RemovePathFromFile( &inputFileNoPathStr );

					string_t inputFileRelative = path_join( Mem_GetTempStorage(), pathFromSolutionToInputFile, inputFileNoPath.data );

					// const char *appPath = Path_RemoveFileFromPath( Path_AppPath() );
					string_t appPath = Path_AppPath( Mem_GetTempStorage() );
					appPath = Path_RemoveFileFromPath( &appPath );
					const char *appPathCStr = String_Cstr( &appPath );

					// build command
					SB_Appendf( &vcxprojContent, "\t\t<NMakeBuildCommandLine>\"%s%c%s\" %s %s%s %s", appPathCStr, PATH_SEPARATOR, BUILDER_PROGRAM_NAME, inputFileRelative.data, ARG_CONFIG, fullConfigName, ARG_VISUAL_STUDIO_BUILD );
					For ( u32, argIndex, 0, config->additionalBuildArgs.size() ) {
						SB_Appendf( &vcxprojContent, " %s", config->additionalBuildArgs[argIndex].c_str() );
					}
					SB_Appendf( &vcxprojContent, "</NMakeBuildCommandLine>\n" );

					// rebuild command
					SB_Appendf( &vcxprojContent, "\t\t<NMakeReBuildCommandLine>\"%s%c%s\" %s %s%s %s", appPathCStr, PATH_SEPARATOR, BUILDER_PROGRAM_NAME, inputFileRelative.data, ARG_CONFIG, fullConfigName, ARG_VISUAL_STUDIO_BUILD );
					For ( u32, argIndex, 0, config->additionalBuildArgs.size() ) {
						SB_Appendf( &vcxprojContent, " %s", config->additionalBuildArgs[argIndex].c_str() );
					}
					SB_Appendf( &vcxprojContent, "</NMakeReBuildCommandLine>\n" );

					// clean comand
					SB_Appendf( &vcxprojContent, "\t\t<NMakeCleanCommandLine>\"%s%c%s\" %s %s</NMakeCleanCommandLine>\n", appPathCStr, PATH_SEPARATOR, BUILDER_PROGRAM_NAME, ARG_NUKE, config->options.binaryFolder.c_str() );

					// preprocessor definitions
					SB_Appendf( &vcxprojContent, "\t\t<NMakePreprocessorDefinitions>" );
					For ( u64, definitionIndex, 0, config->options.defines.size() ) {
						SB_Appendf( &vcxprojContent, "%s;", config->options.defines[definitionIndex].c_str() );
					}
					SB_Appendf( &vcxprojContent, "$(NMakePreprocessorDefinitions)" );
					SB_Appendf( &vcxprojContent, "</NMakePreprocessorDefinitions>\n" );

					SB_Appendf( &vcxprojContent, "\t</PropertyGroup>\n" );
				}
			}

			SB_Appendf( &vcxprojContent, "\t<ItemDefinitionGroup>\n" );
			SB_Appendf( &vcxprojContent, "\t</ItemDefinitionGroup>\n" );

			// tell visual studio what files we have in this project
			// this is typically done via a filter (E.G: src/*.cpp)
			{
				auto WriteFilterFilesToVcxproj = []( stringBuilder_t *stringBuilder, const array_t<visualStudioFileFilter_t>& files, const char* tag) {
					if ( files.count == 0 ) {
						return;
					}

					SB_Appendf( stringBuilder, "\t<ItemGroup>\n" );

					For ( u64, fileIndex, 0, files.count ) {
						SB_Appendf( stringBuilder, "\t\t<%s Include=\"%s\" />\n", tag, files[fileIndex].filePath );
					}

					SB_Appendf( stringBuilder, "\t</ItemGroup>\n" );
				};

				WriteFilterFilesToVcxproj( &vcxprojContent, sourceFiles, "ClCompile" );
				WriteFilterFilesToVcxproj( &vcxprojContent, headerFiles, "ClInclude" );
				WriteFilterFilesToVcxproj( &vcxprojContent, otherFiles, "None" );
			}

			SB_Appendf( &vcxprojContent, "\t<Import Project=\"$(VCTargetsPath)%cMicrosoft.Cpp.targets\" Condition=\"'$(OS)' == 'Windows_NT'\" />\n", PATH_SEPARATOR );

			// not sure what this is or why we need this one but visual studio seems to want it
			SB_Appendf( &vcxprojContent, "\t<ImportGroup Label=\"ExtensionTargets\">\n" );
			SB_Appendf( &vcxprojContent, "\t</ImportGroup>\n" );

			SB_Appendf( &vcxprojContent, "</Project>\n" );

			if ( !WriteStringBuilderToFile( &vcxprojContent, projectPath ) ) {
				return false;
			}

			printf( "Done\n" );
		}

		// .vcxproj.user
		{
			u64 tempPos = Mem_TempTell();
			defer { Mem_TempRewindTo( tempPos ); };

			const char *projectPath = TempPrintf( "%s%c%s.vcxproj.user", visualStudioProjectFilesPath, PATH_SEPARATOR, projectNameNoFolder.data );

			printf( "Generating %s ... ", projectPath );

			stringBuilder_t vcxprojContent = SB_Create( context->allocator );

			SB_Appendf( &vcxprojContent, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
			SB_Appendf( &vcxprojContent, "<Project ToolsVersion=\"Current\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

			SB_Appendf( &vcxprojContent, "\t<PropertyGroup>\n" );
			SB_Appendf( &vcxprojContent, "\t\t<ShowAllFiles>false</ShowAllFiles>\n" );
			SB_Appendf( &vcxprojContent, "\t</PropertyGroup>\n" );

			// for each config and platform, generate the debugger settings
			{
				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig *config = &project->configs[configIndex];

					const char *fullBinaryName = BuildConfig_GetFullBinaryName( &config->options, Mem_GetTempStorage() );

					const char *from = visualStudioProjectFilesPath;
					const char *to = TempPrintf( "%s%c%s", context->inputFilePath.data, PATH_SEPARATOR, fullBinaryName );
					//to = path_canonicalise( to );

					string_t pathFromSolutionToBinaryStr = Path_RelativePathTo( Mem_GetTempStorage(), from, to );

					For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
						const char *platform = options->solution.platforms[platformIndex].c_str();

						SB_Appendf( &vcxprojContent, "\t<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'%s|%s\'\">\n", config->name.c_str(), platform );
						SB_Appendf( &vcxprojContent, "\t\t<DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>\n" );	// TODO(DM): do want to include the other debugger types?
						SB_Appendf( &vcxprojContent, "\t\t<LocalDebuggerDebuggerType>Auto</LocalDebuggerDebuggerType>\n" );
						SB_Appendf( &vcxprojContent, "\t\t<LocalDebuggerAttach>false</LocalDebuggerAttach>\n" );
						SB_Appendf( &vcxprojContent, "\t\t<LocalDebuggerCommand>%s</LocalDebuggerCommand>\n",  String_Cstr( &pathFromSolutionToBinaryStr ) );
						SB_Appendf( &vcxprojContent, "\t\t<LocalDebuggerWorkingDirectory>%s</LocalDebuggerWorkingDirectory>\n", config->runFromDirectory.c_str() );

						// if debugger arguments were specified, put those in
						if ( config->debuggerArguments.size() > 0 ) {
							SB_Appendf( &vcxprojContent, "\t\t<LocalDebuggerCommandArguments>\n" );
							For ( u64, argIndex, 0, config->debuggerArguments.size() ) {
								SB_Appendf( &vcxprojContent, "%s ", config->debuggerArguments[argIndex].c_str() );
							}
							SB_Appendf( &vcxprojContent, "</LocalDebuggerCommandArguments>\n" );
						}

						SB_Appendf( &vcxprojContent, "\t</PropertyGroup>\n" );
					}
				}
			}

			SB_Appendf( &vcxprojContent, "</Project>\n" );

			if ( !WriteStringBuilderToFile( &vcxprojContent, projectPath ) ) {
				return false;
			}

			printf( "Done\n" );
		}

		// .vcxproj.filter
		{
			u64 tempPos = Mem_TempTell();
			defer { Mem_TempRewindTo( tempPos ); };

			const char *projectPath = TempPrintf( "%s%c%s.vcxproj.filters", visualStudioProjectFilesPath, PATH_SEPARATOR, projectNameNoFolder.data );

			printf( "Generating %s ... ", projectPath );

			stringBuilder_t vcxprojContent = SB_Create( context->allocator );

			SB_Appendf( &vcxprojContent, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
			SB_Appendf( &vcxprojContent, "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

			SB_Appendf( &vcxprojContent, "\t<ItemGroup>\n" );

			// write all filter guids
			For ( u64, filterPathIndex, 0, filterPaths.count ) {
				string_t filterPath = Path_FixSlashes( Mem_GetTempStorage(), &filterPaths[filterPathIndex]);

				const char *guid = CreateVisualStudioGuid();

				SB_Appendf( &vcxprojContent, "\t\t<Filter Include=\"%s\">\n", filterPath.data );
				SB_Appendf( &vcxprojContent, "\t\t\t<UniqueIdentifier>{%s}</UniqueIdentifier>\n", guid );
				SB_Appendf( &vcxprojContent, "\t\t</Filter>\n" );
			}

			SB_Appendf( &vcxprojContent, "\t</ItemGroup>\n" );

			// now put all files in the filter
			// visual studio requires that we list each file by type
			{
				auto WriteFileFilters = []( stringBuilder_t *stringBuilder, const array_t<visualStudioFileFilter_t> &fileFilters, const char *tag ) {
					if ( fileFilters.count == 0 ) {
						return;
					}

					SB_Appendf( stringBuilder, "\t<ItemGroup>\n" );

					For ( u64, fileIndex, 0, fileFilters.count ) {
						const visualStudioFileFilter_t* file = &fileFilters[fileIndex];

						if ( file->fileFilter.data == NULL ) {
							SB_Appendf( stringBuilder, "\t\t<%s Include=\"%s\" />\n", tag, file->filePath );
						} else {
							SB_Appendf( stringBuilder, "\t\t<%s Include=\"%s\">\n", tag, file->filePath );
							SB_Appendf( stringBuilder, "\t\t\t<Filter>%s</Filter>\n", file->fileFilter.data );
							SB_Appendf( stringBuilder, "\t\t</%s>\n", tag );
						}
					}

					SB_Appendf( stringBuilder, "\t</ItemGroup>\n" );
				};

				WriteFileFilters( &vcxprojContent, sourceFiles, "ClCompile" );
				WriteFileFilters( &vcxprojContent, headerFiles, "ClInclude" );
				WriteFileFilters( &vcxprojContent, otherFiles, "None" );
			}

			SB_Appendf( &vcxprojContent, "</Project>" );

			if ( !WriteStringBuilderToFile( &vcxprojContent, projectPath ) ) {
				return false;
			}
		}

		printf( "Done\n" );
	}

	// .sln
	{
		u64 tempPos = Mem_TempTell();
		defer { Mem_TempRewindTo( tempPos ); };

		printf( "Generating %s ... ", solutionFilename );

		stringBuilder_t slnContent = SB_Create( context->allocator );

		SB_Appendf( &slnContent, "\n" );
		SB_Appendf( &slnContent, "Microsoft Visual Studio Solution File, Format Version 12.00\n" );
		SB_Appendf( &slnContent, "# Visual Studio Version 17\n" );
		SB_Appendf( &slnContent, "VisualStudioVersion = 17.7.34202.233\n" );		// TODO(DM): how do we query windows for this?
		SB_Appendf( &slnContent, "MinimunVisualStudioVersion = 10.0.40219.1\n" );	// TODO(DM): how do we query windows for this?

		// project GUIDs
		For ( u64, projectIndex, 0, options->solution.projects.size() ) {
			VisualStudioProject *project = &options->solution.projects[projectIndex];

			// const char *projectName = Path_RemovePathFromFile( project->name.c_str() );
			string_t projectNameStr = String_Set( project->name.c_str() );
			string_t projectName = Path_RemovePathFromFile( &projectNameStr );

			SB_Appendf( &slnContent, "Project(\"{%s}\") = \"%s\", \"%s.vcxproj\", \"{%s}\"\n", VISUAL_STUDIO_CPP_PROJECT_TYPE_GUID, projectName.data, projectName.data, projectGuids[projectIndex] );
			SB_Appendf( &slnContent, "EndProject\n" );
		}

		// project folder GUIDs
		For ( u64, projectFolderIndex, 0, projectFolders.count ) {
			const char *folderName = projectFolders[projectFolderIndex];

			u64 folderGuidIndex = options->solution.projects.size() + projectFolderIndex;

			SB_Appendf( &slnContent, "Project(\"{%s}\") = \"%s\", \"%s\", \"{%s}\"\n", VISUAL_STUDIO_FOLDER_PROJECT_TYPE_GUID, folderName, folderName, projectGuids[folderGuidIndex] );
			SB_Appendf( &slnContent, "EndProject\n" );
		}

		SB_Appendf( &slnContent, "Global\n" );
		{
			// which config|platform maps to which config|platform?
			SB_Appendf( &slnContent, "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n" );
			For ( u64, projectIndex, 0, options->solution.projects.size() ) {
				VisualStudioProject *project = &options->solution.projects[projectIndex];

				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig *config = &project->configs[configIndex];

					For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
						const char *platform = options->solution.platforms[platformIndex].c_str();

						SB_Appendf( &slnContent, "\t\t%s|%s = %s|%s\n", config->name.c_str(), platform, config->name.c_str(), platform );
					}
				}
			}
			SB_Appendf( &slnContent, "\tEndGlobalSection\n" );

			// which project config|platform is active?
			SB_Appendf( &slnContent, "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n" );
			For ( u64, projectIndex, 0, options->solution.projects.size() ) {
				VisualStudioProject *project = &options->solution.projects[projectIndex];

				For ( u64, configIndex, 0, project->configs.size() ) {
					VisualStudioConfig *config = &project->configs[configIndex];

					For ( u64, platformIndex, 0, options->solution.platforms.size() ) {
						const char *platform = options->solution.platforms[platformIndex].c_str();

						const char *projectGuid = projectGuids[projectIndex];

						// TODO: the first config and platform in this line are actually the ones that the PROJECT has, not the SOLUTION
						// but we dont use those, and we should
						SB_Appendf( &slnContent, "\t\t{%s}.%s|%s.ActiveCfg = %s|%s\n", projectGuid, config->name.c_str(), platform, config->name.c_str(), platform );
						SB_Appendf( &slnContent, "\t\t{%s}.%s|%s.Build.0 = %s|%s\n", projectGuid, config->name.c_str(), platform, config->name.c_str(), platform );
					}
				}
			}
			SB_Appendf( &slnContent, "\tEndGlobalSection\n" );

			// tell visual studio to not hide the solution node in the Solution Explorer
			// why would you ever want it to be hidden?
			SB_Appendf( &slnContent, "\tGlobalSection(SolutionProperties) = preSolution\n" );
			SB_Appendf( &slnContent, "\t\tHideSolutionNode = FALSE\n" );
			SB_Appendf( &slnContent, "\tEndGlobalSection\n" );

			// which projects are in which project folders (if any)?
			if ( guidParentMappings.count > 0 ) {
				SB_Appendf( &slnContent, "\tGlobalSection(NestedProjects) = preSolution\n" );
				For ( u64, projectIndex, 0, guidParentMappings.count ) {
					guidParentMapping_t *mapping = &guidParentMappings[projectIndex];

					SB_Appendf( &slnContent, "\t\t{%s} = {%s}\n", projectGuids[mapping->guidIndex], projectGuids[mapping->guidParentIndex] );
				}
				SB_Appendf( &slnContent, "\tEndGlobalSection\n" );
			}

			//const char* solutionGUID = CreateVisualStudioGuid();

			//// we need to tell visual studio what the GUID of the solution is, apparently
			//// and we also need to do it in this really roundabout way...for some reason
			//SB_Appendf( &slnContent, "\tGlobalSection(ExtensibilityGlobals) = postSolution\n" );
			//SB_Appendf( &slnContent, "\t\tSolutionGuid = {%s}\n", solutionGUID );
			//SB_Appendf( &slnContent, "\tEndGlobalSection\n" );
		}
		SB_Appendf( &slnContent, "EndGlobal\n" );

		if ( !WriteStringBuilderToFile( &slnContent, solutionFilename ) ) {
			return false;
		}

		printf( "Done\n" );
	}

	printf( "\n" );

	return true;
}

#pragma clang diagnostic pop
