// @third party code - BEGIN HairWorks
#pragma once

class UHairWorksAsset;

/**
* HairWorks component scene proxy.
*/
class ENGINE_API FHairWorksSceneProxy : public FPrimitiveSceneProxy
{
public:
	enum class EDrawType
	{
		Normal,
		Shadow,
		Visualization,
	};

	struct FDynamicRenderData
	{
		NvHw::HairInstanceDescriptor HairInstanceDesc;
		TArray<FMatrix> BoneMatrices;
		FName NormalCenterBoneName;
		TArray<UTexture2D*> Textures;
	};

	FHairWorksSceneProxy(const UPrimitiveComponent* InComponent, UHairWorksAsset& Hair);
	~FHairWorksSceneProxy();
	
	//Begin FPrimitiveSceneProxy interface.
	virtual uint32 GetMemoryFootprint(void) const override;
	virtual void CreateRenderThreadResources() override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)const override;
	//End FPrimitiveSceneProxy interface.

	void UpdateDynamicData_RenderThread(const FDynamicRenderData& DynamicData);

	void Draw(EDrawType DrawType = EDrawType::Normal)const;

	NvHw::HairInstanceId GetHairInstanceId()const { return HairInstanceId; }
	const TArray<FTexture2DRHIRef>& GetTextures()const { return HairTextures; }

protected:
	//** The APEX asset data */
	UHairWorksAsset& Hair;

	//** Bone look up table */
	TMap<FName, int> BoneNameToIdx;

	//** The hair */
	NvHw::HairInstanceId HairInstanceId;

	//** Control textures */
	TArray<FTexture2DRHIRef> HairTextures;
};
// @third party code - END HairWorks
