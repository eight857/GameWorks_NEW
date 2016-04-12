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

	struct FPinMesh
	{
		FMatrix LocalTransform = FMatrix::Identity;	// Relative transform to parent HairWorks component
		FPrimitiveSceneProxy* Mesh = nullptr;
	};

	struct FDynamicRenderData
	{
		NvHair::InstanceDescriptor HairInstanceDesc;
		TArray<UTexture2D*> Textures;
		TArray<TArray<FPinMesh>> PinMeshes;
	};

	FHairWorksSceneProxy(const UPrimitiveComponent* InComponent, NvHair::InstanceId HairInstanceId);
	~FHairWorksSceneProxy();
	
	//~ Begin FPrimitiveSceneProxy interface.
	virtual uint32 GetMemoryFootprint(void) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)const override;
	//~ End FPrimitiveSceneProxy interface.

	void UpdateDynamicData_RenderThread(const FDynamicRenderData& DynamicData);

	void Draw(EDrawType DrawType = EDrawType::Normal)const;

	NvHair::InstanceId GetHairInstanceId()const { return HairInstanceId; }
	const TArray<FTexture2DRHIRef>& GetTextures()const { return HairTextures; }
	const TArray<TArray<FPinMesh>>& GetPinMeshes()const { return HairPinMeshes; }

protected:
	//** The hair */
	NvHair::InstanceId HairInstanceId;

	//** Control textures */
	TArray<FTexture2DRHIRef> HairTextures;

	//** Pin meshes*/
	TArray<TArray<FPinMesh>> HairPinMeshes;
};
// @third party code - END HairWorks
