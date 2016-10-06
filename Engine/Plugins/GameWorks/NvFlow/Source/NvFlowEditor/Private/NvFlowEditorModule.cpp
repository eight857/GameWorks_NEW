#include "NvFlowEditorPCH.h"
#include "PropertyEditorModule.h"

#include "Async.h"

IMPLEMENT_MODULE( FNvFlowEditorModule, NvFlowEditor );
DEFINE_LOG_CATEGORY(LogNvFlowEditor);

inline void FlowRegister()
{
	if (GUnrealEd != nullptr)
	{
		TSharedPtr<FComponentVisualizer> Visualizer = MakeShareable(new FFlowGridComponentVisualizer);

		if (Visualizer.IsValid())
		{
			GUnrealEd->RegisterComponentVisualizer(UFlowGridComponent::StaticClass()->GetFName(), Visualizer);
			Visualizer->OnRegister();
		}
	}
	else
	{
		AsyncTask(ENamedThreads::GameThread, [=]() { FlowRegister(); });
	}
}

void FNvFlowEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	FlowGridAssetTypeActions = MakeShareable(new FAssetTypeActions_FlowGridAsset);
	AssetTools.RegisterAssetTypeActions(FlowGridAssetTypeActions.ToSharedRef());

	FlowRegister();
}

void FNvFlowEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		if (FlowGridAssetTypeActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(FlowGridAssetTypeActions.ToSharedRef());
		}
	}
}
