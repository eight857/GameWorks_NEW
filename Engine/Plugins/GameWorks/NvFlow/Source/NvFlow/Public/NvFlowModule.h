#pragma once

#include "ModuleInterface.h"

struct FNvFlowCommands;
struct RendererHooksNvFlow;

class FNvFlowModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

protected:
	void* FlowModule;
	TUniquePtr<FNvFlowCommands> Commands;

private:
	// Callback function registered with HUD to supply debug info when "ShowDebug NvFlow" has been entered on the console
	static void OnShowDebugInfo(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos);

};

//////////////////////////////////////////////////////////////////////////

