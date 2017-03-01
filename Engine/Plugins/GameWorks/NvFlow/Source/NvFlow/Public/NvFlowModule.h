#pragma once

#include "ModuleInterface.h"

struct FNvFlowCommands;
struct RendererHooksNvFlow;


class FNvFlowDebugInfoQueue
{
public:
	typedef TArray<FString> DebugInfo_t;

	inline DebugInfo_t* GetInfo_Produce() const
	{
		return Info_Produce;
	}

	void SubmitInfo_Produce()
	{
		Info_Produce = (DebugInfo_t*)FPlatformAtomics::InterlockedExchangePtr((void**)&Info_Exchange, Info_Produce);

		InterlockedCompareExchange(&NewInfoFlag, 1, 0);
	}

	DebugInfo_t* FetchInfo_Consume()
	{
		if (InterlockedCompareExchange(&NewInfoFlag, 0, 1) == 1)
		{
			Info_Consume = (DebugInfo_t*)FPlatformAtomics::InterlockedExchangePtr((void**)&Info_Exchange, Info_Consume);
		}
		return Info_Consume;
	}

private:
	DebugInfo_t Info[3];

	DebugInfo_t* Info_Produce = &Info[0];
	DebugInfo_t* Info_Consume = &Info[1];
	volatile DebugInfo_t* Info_Exchange = &Info[2];

	volatile uint32 NewInfoFlag = false;
};
extern FNvFlowDebugInfoQueue NvFlowDebugInfoQueue;


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

