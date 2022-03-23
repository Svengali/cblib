// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

using System.Diagnostics;
using System.Collections.Generic;
using System.IO;

//\build\VS2019\\x64\lib\Pham.lib

//build\VS2019\pham\Debug

public class cblib : ModuleRules
{
	public cblib(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32 ||
			Target.Platform == UnrealTargetPlatform.Mac || Target.Platform == UnrealTargetPlatform.Linux)
		{
			bool bDebug = (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT);

			string LibDir = ModuleDirectory + "/build/";
			string Platform;
			bool bAllowDynamicLibs = true;
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				Platform = "x64";
				//LibDir += "VS" + Target.WindowsPlatform.GetVisualStudioCompilerVersionName() + "/";
				LibDir += "VS2019" + "/";
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				Platform = "Mac";
				bAllowDynamicLibs = false;
			}
			else if (Target.Platform == UnrealTargetPlatform.Linux)
			{ 
				Platform = "Linux";
				bAllowDynamicLibs = false;
			}
			else
			{
				// unsupported
				return;
			}

			LibDir += "/cblib/" + (bDebug ? "Debug":"Release") + "/";  //"/" + "/pham/" + bDebug?"Debug":"Release";

			string LibPostFix = ""; //bDebug && bAllowDynamicLibs ? "d" : "";
			string LibExtension = (Target.Platform == UnrealTargetPlatform.Mac || Target.Platform == UnrealTargetPlatform.Linux) ? ".a" : ".lib";

			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				//PublicDefinitions.Add("H5_BUILT_AS_DYNAMIC_LIB");

				List<string> ReqLibraryNames = new List<string>();

				ReqLibraryNames.AddRange
				(
					new string[] {
						"cblib"
				});

				foreach (string LibraryName in ReqLibraryNames)
				{
					PublicAdditionalLibraries.Add(LibDir + LibraryName + LibPostFix + LibExtension);
				}

				if (Target.bDebugBuildsActuallyUseDebugCRT && bDebug)
				{
					//RuntimeDependencies.Add("$(BinaryOutputDir)/zlibd1.dll", "$(ModuleDir)/Binaries/Win64/zlibd1.dll", StagedFileType.NonUFS);
					//RuntimeDependencies.Add("$(BinaryOutputDir)/hdf5_D.dll", "$(ModuleDir)/Binaries/Win64/hdf5_D.dll", StagedFileType.NonUFS);
				}
				else
				{
					//RuntimeDependencies.Add("$(BinaryOutputDir)/hdf5.dll", "$(ModuleDir)/Binaries/Win64/hdf5.dll", StagedFileType.NonUFS);
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				List<string> ReqLibraryNames = new List<string>();
				ReqLibraryNames.AddRange
				(
					new string[] {
					"libPham"
				  });

				foreach (string LibraryName in ReqLibraryNames)
				{
					PublicAdditionalLibraries.Add(LibDir + LibraryName + LibPostFix + LibExtension);
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				List<string> ReqLibraryNames = new List<string>();
				ReqLibraryNames.AddRange
				(
					new string[] {
					"libPham"
				  });

				foreach (string LibraryName in ReqLibraryNames)
				{
					PublicAdditionalLibraries.Add(LibDir + Target.Architecture + "/" + LibraryName + LibExtension);
				}
			}

			PublicIncludePaths.Add(ModuleDirectory + "/.");

			PublicDependencyModuleNames.Add("UEOpenExr");
		}
	}
}
