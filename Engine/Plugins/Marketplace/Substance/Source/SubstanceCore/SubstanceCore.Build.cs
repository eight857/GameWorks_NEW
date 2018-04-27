// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
using System;
using System.IO;
using System.Collections.Generic;


public class SubstanceCore : ModuleRules
{
	private string ModulePath
	{
		get { return ModuleDirectory; }
	}

	private string PluginRootPath
	{
		get { return Path.GetFullPath(Path.Combine(ModulePath, "../../")); }
	}

	private string SubstanceIncludePath
	{
		get { return Path.GetFullPath(Path.Combine(PluginRootPath, "Include")); }
	}

	public SubstanceCore(ReadOnlyTargetRules Target) : base(Target)
	{
		//Internal defines
		PublicDefinitions.Add("WITH_SUBSTANCE=1");
		PublicDefinitions.Add("SUBSTANCE_PLATFORM_BLEND=1");
		PublicDefinitions.Add("SUBSTANCE_CORE_DEBUG_TOOLS=0");

		//PCH file
		bEnforceIWYU = false;

		//Thread usage
		if (Target.Platform == UnrealTargetPlatform.Win32 ||
		        Target.Platform == UnrealTargetPlatform.Win64 ||
		        Target.Platform == UnrealTargetPlatform.XboxOne)
		{
			PublicDefinitions.Add("AIR_USE_WIN32_SYNC=1");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac ||
		         Target.Platform == UnrealTargetPlatform.PS4 ||
		         Target.Platform == UnrealTargetPlatform.Linux)
		{
			PublicDefinitions.Add("AIR_USE_PTHREAD_SYNC=1");
		}

		//Exposing the include path to substance core publicly. Even though the include paths are public, the libs should no longer be exposed.
		PrivateIncludePaths.Add("SubstanceCore/Private");
		//PrivateIncludePaths.Add("SubstanceCore/PS4");

		//Exposing public include paths
		PublicIncludePaths.Add("SubstanceCore/Public");
		PublicIncludePaths.Add("SubstanceCore/Classes");
		PublicIncludePaths.Add(SubstanceIncludePath);

		//Module dependencies
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Projects",
			"Slate",
			"SlateCore",
			"SubstanceEngine"
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"Core",
			"CoreUObject",
			"Engine",
			"RenderCore",
			"RHI",
			"ImageWrapper",
			"SessionServices",
		});

		//Editor specific configuration
		bool IncludePS4Files = false;

		if (Target.bBuildEditor == true)
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"AssetTools",
				"ContentBrowser",
				"Settings",
				"TargetPlatform",
				"MainFrame"
			});

			//Used for ps4 cooking
			string SDKDir = System.Environment.GetEnvironmentVariable("SCE_ORBIS_SDK_DIR");
			if ((SDKDir != null) && (SDKDir.Length > 0))
			{
				PublicIncludePaths.Add(SDKDir + "/target/include_common");
				PublicLibraryPaths.Add(SDKDir + "/host_tools/lib");

				PublicAdditionalLibraries.Add("libSceGpuAddress.lib");
				PublicDelayLoadDLLs.Add("libSceGpuAddress.dll");
				PublicAdditionalLibraries.Add("libSceGnm.lib");
				PublicDelayLoadDLLs.Add("libSceGnm.dll");

				//Toggle on our flag if we are building for PS4
				IncludePS4Files = true;
			}
		}

		//Overwrite PS4 SDK if the files don't exist - Check both Engine and Project directories
		string ConsoleFilePath = "SubstanceCore\\Private\\SubstanceCoreConsoleSpecific.h";
		string BaseEnginePath = Path.Combine("..", "Plugins", "Runtime", "Substance", "Source");
		string BaseProjectPath = Path.Combine("Plugins", "Runtime", "Substance", "Source");
		string ConsoleEngineFilePath = Path.Combine(BaseEnginePath, ConsoleFilePath);
		string ProjectParentDir = ModuleDirectory;
		//string ProjectParentDir = System.IO.Directory.GetParent(Target.ProjectFile.ToString()).ToString();
		string ConsoleProjectFilePath = Path.Combine(ProjectParentDir, BaseProjectPath, ConsoleFilePath);

		//Check both possible locations of where the console files could be
		bool ConsoleFilesFoundInEnginePluginDir = File.Exists(ConsoleEngineFilePath) && IncludePS4Files;
		bool ConsoleFilesFoundInProjectPluginDir = File.Exists(ConsoleProjectFilePath) && IncludePS4Files;

		//Craft the project file location to test as well
		IncludePS4Files = (ConsoleFilesFoundInEnginePluginDir || ConsoleFilesFoundInProjectPluginDir) && IncludePS4Files;

		PublicDefinitions.Add("SUBSTANCE_HAS_PS4_SDK=" + (IncludePS4Files ? "1" : "0"));

		if (IncludePS4Files)
			UnrealBuildTool.Log.WriteLine(LogEventType.Log, "Substance Editor Plugin: PS4 Cooking Enabled");
	}
}

