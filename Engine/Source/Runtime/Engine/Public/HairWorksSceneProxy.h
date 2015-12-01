// @third party code - BEGIN HairWorks
#pragma once

enum GFSDK_HairInstanceID;
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
		GFSDK_HairInstanceDescriptor HairInstanceDesc;
		TArray<FMatrix> BoneMatrices;
		FName NormalCenterBoneName;
		TArray<UTexture2D*> Textures;
	};

	FHairWorksSceneProxy(const UPrimitiveComponent* InComponent, UHairWorksAsset& Hair);
	~FHairWorksSceneProxy();
	
	//Begin FPrimitiveSceneProxy interface.
	virtual uint32 GetMemoryFootprint(void) const override;
	virtual void CreateRenderThreadResources() override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) override;
	//End FPrimitiveSceneProxy interface.

	void UpdateDynamicData_RenderThread(const FDynamicRenderData& DynamicData);

	void Draw(EDrawType DrawType = EDrawType::Normal)const;

	GFSDK_HairInstanceID GetHairInstanceId()const { return HairInstanceId; }
	const TArray<FTexture2DRHIRef>& GetTextures()const { return HairTextures; }

protected:
	//** The APEX asset data */
	UHairWorksAsset& Hair;

	//** Bone look up table */
	TMap<FName, int> BoneNameToIdx;

	//** The hair */
	GFSDK_HairInstanceID HairInstanceId;

	//** Control textures */
	TArray<FTexture2DRHIRef> HairTextures;
};
// @third party code - END HairWorks
