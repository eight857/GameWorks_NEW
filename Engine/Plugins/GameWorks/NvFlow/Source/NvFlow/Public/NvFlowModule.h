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
};

//////////////////////////////////////////////////////////////////////////

