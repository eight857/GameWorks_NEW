// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"
#include "HairWorksSDK.h"
#include "Engine/HairWorksMaterial.h"

UHairWorksMaterial::UHairWorksMaterial(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UHairWorksMaterial::SyncHairDescriptor(GFSDK_HairInstanceDescriptor& HairDescriptor, TArray<UTexture2D*>& HairTextures, bool bFromDescriptor)
{
	HairTextures.SetNum(GFSDK_HAIR_NUM_TEXTURES, false);

#pragma region Visualization
	auto SyncHairVisualizationFlag = [&](bool& HairFlag, bool& Property)
	{
		if(bFromDescriptor)
			Property = HairFlag;
		else
			HairFlag |= Property;
	};

	if(bFromDescriptor)
		bHair = HairDescriptor.m_drawRenderHairs;
	else
		HairDescriptor.m_drawRenderHairs &= bHair;
	SyncHairVisualizationFlag(HairDescriptor.m_visualizeBones, bBones);
	SyncHairVisualizationFlag(HairDescriptor.m_visualizeBoundingBox, bBoundingBox);
	SyncHairVisualizationFlag(HairDescriptor.m_visualizeCapsules, bCollisionCapsules);
	SyncHairVisualizationFlag(HairDescriptor.m_visualizeControlVertices, bControlPoints);
	SyncHairVisualizationFlag(HairDescriptor.m_visualizeGrowthMesh, bGrowthMesh);
	SyncHairVisualizationFlag(HairDescriptor.m_visualizeGuideHairs, bGuideCurves);
	SyncHairVisualizationFlag(HairDescriptor.m_visualizeHairInteractions, bHairInteraction);
	SyncHairVisualizationFlag(HairDescriptor.m_visualizePinConstraints, bPinConstraints);
	SyncHairVisualizationFlag(HairDescriptor.m_visualizeShadingNormals, bShadingNormal);
	SyncHairVisualizationFlag(HairDescriptor.m_visualizeShadingNormalBone, bShadingNormalCenter);
	SyncHairVisualizationFlag(HairDescriptor.m_visualizeSkinnedGuideHairs, bSkinnedGuideCurves);
	if(bFromDescriptor)
		ColorizeOptions = static_cast<EHairWorksColorizeMode>(HairDescriptor.m_colorizeMode);
	else if(HairDescriptor.m_colorizeMode == GFSDK_HAIR_COLORIZE_MODE_NONE)
		HairDescriptor.m_colorizeMode = static_cast<unsigned>(ColorizeOptions);
#pragma endregion

#pragma region General
	SyncHairParameter(HairDescriptor.m_enable, bEnable, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_splineMultiplier, SplineMultiplier, bFromDescriptor);
#pragma endregion

#pragma region Physical
	SyncHairParameter(HairDescriptor.m_simulate, bSimulate, bFromDescriptor);
	FVector GravityDir = FVector(0, 0, -1);
	SyncHairParameter(HairDescriptor.m_gravityDir, (gfsdk_float3&)GravityDir, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_massScale, MassScale, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_damping, Damping, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_inertiaScale, InertiaScale, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_inertiaLimit, InertiaLimit, bFromDescriptor);
#pragma endregion

#pragma region Wind
	FVector WindVector = WindDirection.Vector() * Wind;
	SyncHairParameter(HairDescriptor.m_wind, (gfsdk_float3&)WindVector, bFromDescriptor);
	if(bFromDescriptor)
	{
		Wind = WindVector.Size();
		WindDirection = FRotator(FQuat(FRotationMatrix::MakeFromX(WindVector)));
	}
	SyncHairParameter(HairDescriptor.m_windNoise, WindNoise, bFromDescriptor);
#pragma endregion

#pragma region Stiffness
	SyncHairParameter(HairDescriptor.m_stiffness, StiffnessGlobal, bFromDescriptor);
	SyncHairParameter(HairTextures[GFSDK_HAIR_TEXTURE_STIFFNESS], StiffnessGlobalMap, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_stiffnessCurve, (gfsdk_float4&)StiffnessGlobalCurve, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_stiffnessStrength, StiffnessStrength, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_stiffnessStrengthCurve, (gfsdk_float4&)StiffnessStrengthCurve, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_stiffnessDamping, StiffnessDamping, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_stiffnessDampingCurve, (gfsdk_float4&)StiffnessDampingCurve, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_rootStiffness, StiffnessRoot, bFromDescriptor);
	SyncHairParameter(HairTextures[GFSDK_HAIR_TEXTURE_ROOT_STIFFNESS], StiffnessRootMap, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_tipStiffness, StiffnessTip, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_bendStiffness, StiffnessBend, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_bendStiffnessCurve, (gfsdk_float4&)StiffnessBendCurve, bFromDescriptor);
#pragma endregion

#pragma region Collision
	SyncHairParameter(HairDescriptor.m_backStopRadius, Backstop, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_friction, Friction, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_useCollision, bCapsuleCollision, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_interactionStiffness, StiffnessInteraction, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_interactionStiffnessCurve, (gfsdk_float4&)StiffnessInteractionCurve, bFromDescriptor);
#pragma endregion

#pragma region Pin
	SyncHairParameter(HairDescriptor.m_pinStiffness, PinStiffness, bFromDescriptor);
#pragma endregion

#pragma region Volume
	SyncHairParameter(HairDescriptor.m_density, Density, bFromDescriptor);
	SyncHairParameter(HairTextures[GFSDK_HAIR_TEXTURE_DENSITY], DensityMap, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_usePixelDensity, bUsePixelDensity, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_lengthScale, LengthScale, bFromDescriptor);
	SyncHairParameter(HairTextures[GFSDK_HAIR_TEXTURE_LENGTH], LengthScaleMap, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_lengthNoise, LengthNoise, bFromDescriptor);
#pragma endregion

#pragma region Strand Width
	SyncHairParameter(HairDescriptor.m_width, WidthScale, bFromDescriptor);
	SyncHairParameter(HairTextures[GFSDK_HAIR_TEXTURE_WIDTH], WidthScaleMap, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_widthRootScale, WidthRootScale, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_widthTipScale, WidthTipScale, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_widthNoise, WidthNoise, bFromDescriptor);
#pragma endregion

#pragma region Clumping
	SyncHairParameter(HairDescriptor.m_clumpScale, ClumpingScale, bFromDescriptor);
	SyncHairParameter(HairTextures[GFSDK_HAIR_TEXTURE_CLUMP_SCALE], ClumpingScaleMap, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_clumpRoundness, ClumpingRoundness, bFromDescriptor);
	SyncHairParameter(HairTextures[GFSDK_HAIR_TEXTURE_CLUMP_ROUNDNESS], ClumpingRoundnessMap, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_clumpNoise, ClumpingNoise, bFromDescriptor);
#pragma endregion

#pragma region Waveness
	SyncHairParameter(HairDescriptor.m_waveScale, WavinessScale, bFromDescriptor);
	SyncHairParameter(HairTextures[GFSDK_HAIR_TEXTURE_WAVE_SCALE], WavinessScaleMap, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_waveScaleNoise, WavinessScaleNoise, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_waveScaleStrand, WavinessScaleStrand, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_waveScaleClump, WavinessScaleClump, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_waveFreq, WavinessFreq, bFromDescriptor);
	SyncHairParameter(HairTextures[GFSDK_HAIR_TEXTURE_WAVE_FREQ], WavinessFreqMap, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_waveFreqNoise, WavinessFreqNoise, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_waveRootStraighten, WavinessRootStraigthen, bFromDescriptor);
#pragma endregion

#pragma region Color
	SyncHairParameter(HairDescriptor.m_rootColor, (gfsdk_float4&)RootColor, bFromDescriptor);
	SyncHairParameter(HairTextures[GFSDK_HAIR_TEXTURE_ROOT_COLOR], RootColorMap, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_tipColor, (gfsdk_float4&)TipColor, bFromDescriptor);
	SyncHairParameter(HairTextures[GFSDK_HAIR_TEXTURE_TIP_COLOR], TipColorMap, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_rootTipColorWeight, RootTipColorWeight, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_rootTipColorFalloff, RootTipColorFalloff, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_rootAlphaFalloff, RootAlphaFalloff, bFromDescriptor);
#pragma endregion

#pragma region Strand
	SyncHairParameter(HairTextures[GFSDK_HAIR_TEXTURE_STRAND], PerStrandTexture, bFromDescriptor);

	switch(StrandBlendMode)
	{
	case EHairWorksStrandBlendMode::Overwrite:
		HairDescriptor.m_strandBlendMode = GFSDK_HAIR_STRAND_BLEND_OVERWRITE;
		break;
	case EHairWorksStrandBlendMode::Multiply:
		HairDescriptor.m_strandBlendMode = GFSDK_HAIR_STRAND_BLEND_MULTIPLY;
		break;
	case EHairWorksStrandBlendMode::Add:
		HairDescriptor.m_strandBlendMode = GFSDK_HAIR_STRAND_BLEND_ADD;
		break;
	case EHairWorksStrandBlendMode::Modulate:
		HairDescriptor.m_strandBlendMode = GFSDK_HAIR_STRAND_BLEND_MODULATE;
		break;
	}

	SyncHairParameter(HairDescriptor.m_strandBlendScale, StrandBlendScale, bFromDescriptor);
#pragma endregion

#pragma region Diffuse
	SyncHairParameter(HairDescriptor.m_diffuseBlend, DiffuseBlend, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_hairNormalWeight, HairNormalWeight, bFromDescriptor);
#pragma endregion

#pragma region Specular
	SyncHairParameter(HairDescriptor.m_specularColor, (gfsdk_float4&)SpecularColor, bFromDescriptor);
	SyncHairParameter(HairTextures[GFSDK_HAIR_TEXTURE_SPECULAR], SpecularColorMap, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_specularPrimary, PrimaryScale, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_specularPowerPrimary, PrimaryShininess, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_specularPrimaryBreakup, PrimaryBreakup, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_specularSecondary, SecondaryScale, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_specularPowerSecondary, SecondaryShininess, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_specularSecondaryOffset, SecondaryOffset, bFromDescriptor);
#pragma endregion

#pragma region Glint
	SyncHairParameter(HairDescriptor.m_glintStrength, GlintStrength, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_glintCount, GlintSize, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_glintExponent, GlintPowerExponent, bFromDescriptor);
#pragma endregion

#pragma region Shadow
	SyncHairParameter(HairDescriptor.m_shadowSigma, ShadowAttenuation, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_shadowDensityScale, ShadowDensityScale, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_castShadows, bCastShadows, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_receiveShadows, bReceiveShadows, bFromDescriptor);
#pragma endregion

#pragma region Culling
	SyncHairParameter(HairDescriptor.m_useViewfrustrumCulling, bViewFrustumCulling, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_useBackfaceCulling, bBackfaceCulling, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_backfaceCullingThreshold, BackfaceCullingThreshold, bFromDescriptor);
#pragma endregion

#pragma region LOD
	if(!bFromDescriptor)
		HairDescriptor.m_enableLOD = true;
#pragma endregion

#pragma region Distance LOD
	SyncHairParameter(HairDescriptor.m_enableDistanceLOD, bDistanceLodEnable, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_distanceLODStart, DistanceLodStart, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_distanceLODEnd, DistanceLodEnd, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_distanceLODFadeStart, FadeStartDistance, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_distanceLODWidth, DistanceLodBaseWidthScale, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_distanceLODDensity, DistanceLodBaseDensityScale, bFromDescriptor);
#pragma endregion

#pragma region Detail LOD
	SyncHairParameter(HairDescriptor.m_enableDetailLOD, bDetailLodEnable, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_detailLODStart, DetailLodStart, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_detailLODEnd, DetailLodEnd, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_detailLODWidth, DetailLodBaseWidthScale, bFromDescriptor);
	SyncHairParameter(HairDescriptor.m_detailLODDensity, DetailLodBaseDensityScale, bFromDescriptor);
#pragma endregion
}
// @third party code - END HairWorks
