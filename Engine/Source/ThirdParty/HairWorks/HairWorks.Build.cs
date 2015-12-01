// @third party code - BEGIN HairWorks
using UnrealBuildTool;

public class HairWorks : ModuleRules
{
	public HairWorks(TargetInfo Target)
	{
		Type = ModuleType.External;

		// Add DLL to package
		string PlatformString = null;
		switch (Target.Platform)
		{
			case UnrealTargetPlatform.Win32:
				PlatformString = "win32";
				break;
			case UnrealTargetPlatform.Win64:
				PlatformString = "win64";
				break;
		}

		if (PlatformString != null)
		{
			string DllPath = "$(EngineDir)/Binaries/ThirdParty/HairWorks/GFSDK_HairWorks." + PlatformString + ".dll";
			RuntimeDependencies.Add(new RuntimeDependency(DllPath));
		}
	}
}
// @third party code - END HairWorks


