// NVCHANGE_BEGIN: Add VXGI
using UnrealBuildTool;

public class VXGI : ModuleRules
{
	public VXGI(ReadOnlyTargetRules Target) : base(Target)
    {
		Type = ModuleType.External;

		if (Target.Platform != UnrealTargetPlatform.Win64)
		{
			// We only ship the x64 build of VXGI
			Definitions.Add("WITH_GFSDK_VXGI=0"); // avoid the warnings on undefined symbol
			return;
		}

		Definitions.Add("WITH_GFSDK_VXGI=1");

		string VXGIDir = Target.UEThirdPartySourceDirectory + "GameWorks/VXGI";
		PublicIncludePaths.Add(VXGIDir + "/include");

		string ArchName = "x64";
		string DebugSuffix = "";

		PublicLibraryPaths.Add(VXGIDir + "/lib/" + ArchName);
		PublicAdditionalLibraries.Add("GFSDK_VXGI" + DebugSuffix + "_" + ArchName + ".lib");
		PublicDelayLoadDLLs.Add("GFSDK_VXGI" + DebugSuffix + "_" + ArchName + ".dll");
	}
}
// NVCHANGE_END: Add VXGI
