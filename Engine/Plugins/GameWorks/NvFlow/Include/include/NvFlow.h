/*
 * Copyright (c) 2008-2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once

#include "NvFlowContext.h"

#define NV_FLOW_VERSION 0

// --------------------------- NvFlowContext -------------------------------
///@defgroup NvFlowContext
///@{

/**
 * Creates a graphics/compute context for Flow.
 * 
 * @param[in] version Should be set by app to NV_FLOW_VERSION.
 * @param[in] desc A graphics-API dependent structure containing data needed for a FlowContext to interoperate with the app.
 *
 * @return The created Flow context.
 */
NV_FLOW_API NvFlowContext* NvFlowCreateContext(NvFlowUint version, const NvFlowContextDesc* desc);

/**
 * Releases a Flow context.
 *
 * @param[in] context The Flow context to be released.
 */
NV_FLOW_API void NvFlowReleaseContext(NvFlowContext* context);

/**
 * Creates a Flow depth stencil view based on information provided by the application.
 * 
 * @param[in] context The Flow context to create and use the depth stencil view.
 * @param[in] desc The graphics API dependent description.
 *
 * @return The created Flow depth stencil view.
 */
NV_FLOW_API NvFlowDepthStencilView* NvFlowCreateDepthStencilView(NvFlowContext* context, const NvFlowDepthStencilViewDesc* desc);

/**
 * Releases a Flow depth stencil view.
 *
 * @param[in] view The Flow depth stencil view to be released.
 */
NV_FLOW_API void NvFlowReleaseDepthStencilView(NvFlowDepthStencilView* view);

/**
 * Creates a Flow render target view based on information provided by the application.
 *
 * @param[in] context The Flow context to create and use the render target view.
 * @param[in] desc The graphics API dependent description.
 *
 * @return The created Flow render target view.
 */
NV_FLOW_API NvFlowRenderTargetView* NvFlowCreateRenderTargetView(NvFlowContext* context, const NvFlowRenderTargetViewDesc* desc);

/**
 * Releases a Flow render target view.
 *
 * @param[in] view The Flow render target view to be released.
 */
NV_FLOW_API void NvFlowReleaseRenderTargetView(NvFlowRenderTargetView* view);

/**
 * Updates a Flow context with information provided by the application.
 *
 * @param[in] context The Flow context to update.
 * @param[in] desc The graphics API dependent description.
 */
NV_FLOW_API void NvFlowUpdateContext(NvFlowContext* context, const NvFlowContextDesc* desc);

/**
 * Updates a Flow depth stencil view with information provided by the application.
 *
 * @param[in] context The Flow context used to create the depth stencil view.
 * @param[in] view The Flow depth stencil view to update.
 * @param[in] desc The graphics API dependent description.
 */
NV_FLOW_API void NvFlowUpdateDepthStencilView(NvFlowContext* context, NvFlowDepthStencilView* view, const NvFlowDepthStencilViewDesc* desc);

/**
 * Updates a Flow render target view with information provided by the application.
 *
 * @param[in] context The Flow context used to create the render target view.
 * @param[in] view The Flow render target view to update.
 * @param[in] desc The graphics API dependent description.
 */
NV_FLOW_API void NvFlowUpdateRenderTargetView(NvFlowContext* context, NvFlowRenderTargetView* view, const NvFlowRenderTargetViewDesc* desc);

/**
 * Updates an application visible description with internal Flow resource information.
 *
 * @param[in] context The Flow context that created the resource.
 * @param[in] resource The Flow resource to describe.
 * @param[out] desc The graphics API dependent Flow resource description.
 */
NV_FLOW_API void NvFlowUpdateResourceViewDesc(NvFlowContext* context, NvFlowResource* resource, NvFlowResourceViewDesc* desc);

/**
 * Updates an application visible description with internal Flow resourceRW information.
 *
 * @param[in] context The Flow context that created the resourceRW.
 * @param[in] buffer The Flow resourceRW to describe.
 * @param[out] desc The graphics API dependent Flow resourceRW description.
 */
NV_FLOW_API void NvFlowUpdateResourceRWViewDesc(NvFlowContext* context, NvFlowResourceRW* resourceRW, NvFlowResourceRWViewDesc* desc);

/**
 * Pushes graphics/compute pipeline state for later restoration by NvFlowContextPop.
 *
 * @param[in] context The Flow context to push.
 */
NV_FLOW_API void NvFlowContextPush(NvFlowContext* context);

/**
 * Restores graphics/compute pipeline state pushed by NvFlowContextPush.
 *
 * @param[in] context The Flow context to restore.
 */
NV_FLOW_API void NvFlowContextPop(NvFlowContext* context);

/**
 * An optional callback to allow the application to control how Flow allocates CPU memory.
 *
 * @param[in] malloc The allocation function for Flow to use.
 */
NV_FLOW_API void NvFlowSetMallocFunc(void*(*malloc)(size_t size));

/**
 * An optional callback to allow the application to control how Flow releases CPU memory.
 *
 * @param[in] free The free function for Flow to use.
 */
NV_FLOW_API void NvFlowSetFreeFunc(void(*free)(void* ptr));

///@}
// -------------------------- NvFlowGrid -------------------------------
///@defgroup NvFlowGrid
///@{

//! A Flow dynamic grid
struct NvFlowGrid;

//! Grid data for rendering
struct NvFlowGridView;

//! Grid simulation channel
enum NvFlowGridChannel
{
	eNvFlowGridChannelVelocity = 0,
	eNvFlowGridChannelDensity = 1
};

//! Enumeration used to describe density resolution relative to velocity resolution
enum NvFlowMultiRes
{
	eNvFlowMultiRes1x1x1 = 0,
	eNvFlowMultiRes2x2x2 = 1
};

//! Description required to create a NvFlowGrid
struct NvFlowGridDesc
{
	NvFlowFloat3 initialLocation;		//!< Initial location of axis aligned bounding box
	NvFlowFloat3 halfSize;				//!< Initial half size of axis aligned bounding box

	NvFlowDim virtualDim;				//!< Resolution of virtual address space inside of bounding box
	NvFlowMultiRes densityMultiRes;		//!< Number of density cells per velocity cell

	float residentScale;				//!< Fraction of virtual cells to allocate memory for

	bool enableVTR;						//!< Enable use of volume tiled resources, if supported
	bool lowLatencyMapping;				//!< Faster mapping updates, more mapping overhead but less prediction required
};

/**
 * Allows the application to request a default grid description from Flow.
 *
 * @param[out] desc The description for Flow to fill out.
 */
NV_FLOW_API void NvFlowGridDescDefaults(NvFlowGridDesc* desc);

//! Grid wide parameters controls combustion behavior
struct NvFlowGridCombustionParams
{
	float ignitionTemp;			//!< Minimum temperature for combustion
	float burnPerTemp;			//!< Burn amount per unit temperature above ignitionTemp
	float fuelPerBurn;			//!< Fuel consumed per unit burn
	float tempPerBurn;			//!< Temperature increase per unit burn
	float densityPerBurn;		//!< Density increase per unit burn
	float divergencePerBurn;	//!< Expansion per unit burn
	float buoyancyPerTemp;		//!< Buoyant force per unit temperature
	float coolingRate;			//!< Cooling rate, exponential
};

//! Parameters controlling grid behavior
struct NvFlowGridParams
{
	float velocityDamping;					//!< Higher values reduce velocity faster (exponential decay curve)
	float densityDamping;					//!< Higher values reduce density faster (exponential decay curve)
	float fuelDamping;						//!< Higher values reduce fuel faster (exponential decay curve)
	float velocityFade;						//!< Fade velocity rate in units / sec
	float densityFade;						//!< Fade density rate in units / sec
	float fuelFade;							//!< Fade fuel rate in units / sec

	float velocityMacCormackBlendFactor;	//!< Higher values make a sharper appearance, but with more artifacts
	float densityMacCormackBlendFactor;		//!< Higher values make a sharper appearance, but with more artifacts

	NvFlowFloat3 gravity;					//!< Gravity vector for use by buoyancy

	NvFlowGridCombustionParams combustion;	//!< Combustion parameters

	float vorticityStrength;				//!< Higher values increase rotation, reduce laminar flow

	float velocityWeight;					//!< Relative importance of velocity for allocation, 0.0 means not important
	float densityWeight;					//!< Relative importance of density for allocation, 0.0 means not important
	float tempWeight;						//!< Relative importance of temperature for allocation, 0.0 means not important
	float fuelWeight;						//!< Relative importance of fuel for allocation, 0.0 means not important

	float velocityThreshold;				//!< Minimum velocity magnitude that is considered relevant
	float densityThreshold;					//!< Minimum density magnitude that is considered relevant
	float tempThreshold;					//!< Minimum temperature magnitude that is considered relevant
	float fuelThreshold;					//!< Minimum fuel magnitude that is considered relevant

	float importanceThreshold;				//!< Global importance threshold, to scale quality/performance
};

/**
 * Allows the application to request default grid parameters from Flow.
 *
 * @param[out] params The parameters for Flow to fill out.
 */
NV_FLOW_API void NvFlowGridParamsDefaults(NvFlowGridParams* params);

//! Types of shapes for emit/collide behavior
enum NvFlowShapeType
{
	eNvFlowShapeTypeSDF = 0,
	eNvFlowShapeTypeSphere = 1,
	eNvFlowShapeTypeBox = 2,
	eNvFlowShapeTypeCapsule = 3,
	eNvFlowShapeTypePlane = 4
};

//! A signed distance field shape object for emitters and/or collision
struct NvFlowShapeSDF;

//! Description of a signed distance field shape
struct NvFlowShapeDescSDF
{
	NvFlowShapeSDF* sdf;	//!< Pointer to signed distance field object
};

//! Desription of a sphere
struct NvFlowShapeDescSphere
{
	float radius;			//!< Radius in local space
};

//! Description of a box
struct NvFlowShapeDescBox
{
	NvFlowFloat3 halfSize;	//!< HalfSize in local space
};

//! Description of a capsule
struct NvFlowShapeDescCapsule
{
	float radius;			//!< Radius in local space
	float length;			//!< Length in local space on x axis
};

//! Description of a plane
struct NvFlowShapeDescPlane
{
	NvFlowFloat3 normal;	//!< Normal vector of the plane in local space
	float distance;			//!< Shortest signed distance from the origin to the plane in local space
};

//! Shared type for shape descriptions
union NvFlowShapeDesc
{
	NvFlowShapeDescSDF sdf;
	NvFlowShapeDescSphere sphere;
	NvFlowShapeDescBox box;
	NvFlowShapeDescCapsule capsule;
	NvFlowShapeDescPlane plane;
};

//! Parameters for both emission and collision
struct NvFlowGridEmitParams
{
	NvFlowUint shapeRangeOffset;					//!< Start of shape range, offset in number of Shapes
	NvFlowUint shapeRangeSize;						//!< Size of shape range, in number of Shapes
	NvFlowShapeType shapeType;						//!< Type of shape in the set
	float shapeDistScale;							//!< Scale to apply to SDF value

	float deltaTime;								//!< DeltaTime used to compute impulse

	NvFlowFloat4x4 bounds;							//!< Transform from emitter ndc to world space
	NvFlowFloat4x4 localToWorld;					//!< Transform from shape local space to world space

	float minActiveDist;							//!< Minimum distance value for active emission
	float maxActiveDist;							//!< Maximum distance value for active emission
	float slipThickness;							//!< Thickness of slip boundary region
	float slipFactor;								//!< 0.0 => no slip, fully damped; 1.0 => full slip

	NvFlowFloat3 centerOfMass; 						//!< Center of mass in emitter ndc coordinate space

	NvFlowFloat3 velocityLinear;					//!< Linear velocity, in world units, emitter direction
	NvFlowFloat3 velocityAngular;					//!< Angular velocity, in world units, emitter direction

	NvFlowFloat3 velocityCoupleRate;				//!< Rate of correction to target, inf means instantaneous

	float temperature;								//!< Target temperature
	float temperatureCoupleRate;					//!< Rate of correction to target, inf means instantaneous

	float fuel;										//!< Target fuel
	float fuelCoupleRate;							//!< Rate of correction to target, inf means instantaneous
	float fuelReleaseTemp;							//!< Minimum temperature to release fuelRelease additional fuel
	float fuelRelease;								//!< Fuel released when temperature exceeds release temperature

	float density;									//!< Target density
	float densityCoupleRate;						//!< Rate of correction to target, inf means instantaneous

	NvFlowFloat3 allocationScale;					//!< Higher values cause more blocks to allocate around emitter; 0.f means no allocation, 1.f is default
	float allocationPredict;						//!< Higher values cause extra allocation based on linear velocity and predict velocity
	NvFlowFloat3 predictVelocity;					//!< Velocity used only for predict
	float predictVelocityWeight;					//!< Blend weight between linearVelocity and predictVelocity

	NvFlowUint numSubSteps;							//!< Numbers of interations to perform on cell value
};

/**
 * Allows the application to request default emit parameters from Flow.
 *
 * @param[out] params The parameters for Flow to fill out.
 */
NV_FLOW_API void NvFlowGridEmitParamsDefaults(NvFlowGridEmitParams* params);

//! Description required to create a signed distance field object.
struct NvFlowShapeSDFDesc
{
	NvFlowDim resolution;		//!< The resolution of the 3D texture used to store the signed distance field.
};

/**
 * Allows the application to request a default signed distance field object description from Flow.
 *
 * @param[out] desc The description for Flow to fill out.
 */
NV_FLOW_API void NvFlowShapeSDFDescDefaults(NvFlowShapeSDFDesc* desc);

//! Required information for writing to a CPU mapped signed distance field.
struct NvFlowShapeSDFData
{
	float* data;				//!< Pointer to mapped data
	NvFlowUint rowPitch;		//!< Row pitch in floats
	NvFlowUint depthPitch;		//!< Depth pitch in floats
	NvFlowDim dim;				//!< Dimension of the sdf texture
};

/**
 * Creates a signed distance field object with no initial data.
 *
 * @param[in] context The Flow context to use for creation.
 * @param[in] desc A description needed for memory allocation.
 * 
 * @return The created signed distance field object.
 */
NV_FLOW_API NvFlowShapeSDF* NvFlowCreateShapeSDF(NvFlowContext* context, const NvFlowShapeSDFDesc* desc);

/**
 * Creates a signed distance field object with data from a Flow 3D texture.
 *
 * @param[in] context The Flow context to use for creation.
 * @param[in] texture The Flow 3D texture containing the signed distance field to use.
 *
 * @return The created signed distance field object.
 */
NV_FLOW_API NvFlowShapeSDF* NvFlowCreateShapeSDFFromTexture3D(NvFlowContext* context, NvFlowTexture3D* texture);

/**
 * Releases a Flow signed distance field object.
 *
 * @param[in] shape The Flow signed distance field to be released.
 */
NV_FLOW_API void NvFlowReleaseShapeSDF(NvFlowShapeSDF* shape);

/**
 * Maps a signed distance field object for CPU write access.
 *
 * @param[in] shape The Flow signed distance field object to map.
 * @param[in] context The Flow context used to create the Flow signed distance field.
 *
 * @return The information needed to properly write to the mapped signed distance field object.
 */
NV_FLOW_API NvFlowShapeSDFData NvFlowShapeSDFMap(NvFlowShapeSDF* shape, NvFlowContext* context);

/**
 * Unmaps a signed distance field object from CPU write access, uploads update field to GPU.
 *
 * @param[in] shape The Flow signed distance field object to unmap.
 * @param[in] context The Flow context used to create the Flow signed distance field.
 */
NV_FLOW_API void NvFlowShapeSDFUnmap(NvFlowShapeSDF* shape, NvFlowContext* context);

//! Description of feature support on the queried Flow context GPU.
struct NvFlowSupport
{
	bool supportsVTR;		//!< True if volume tiled resources are supported
};

//! CPU/GPU timing info
struct NvFlowQueryTime
{
	float simulation;
};

/**
 * Creates a Flow grid.
 *
 * @param[in] context The Flow context used to create the new Flow grid
 * @param[in] desc The Flow grid description.
 *
 * @return The created Flow grid
 */
NV_FLOW_API NvFlowGrid* NvFlowCreateGrid(NvFlowContext* context, const NvFlowGridDesc* desc);

/**
 * Releases a Flow grid.
 *
 * @param[in] grid The Flow grid to be released.
 */
NV_FLOW_API void NvFlowReleaseGrid(NvFlowGrid* grid);

/**
 * Queries support for features that depend on hardware/OS.
 *
 * @param[in] grid The Flow grid to query for support.
 * @param[out] support Description of what is supported.
 *
 * @return Returns eNvFlowSuccess if information is available.
 */
NV_FLOW_API NvFlowResult NvFlowGridQuerySupport(NvFlowGrid* grid, NvFlowSupport* support);

/**
 * Queries simulation timing data.
 *
 * @param[in] grid The Flow grid to query for timing.
 * @param[out] gpuTime Simulation overhead on GPU.
 * @param[out] cpuTime Simulation overhead on CPU.
 *
 * @return Returns eNvFlowSuccess if information is available.
 */
NV_FLOW_API NvFlowResult NvFlowGridQueryTime(NvFlowGrid* grid, NvFlowQueryTime* gpuTime, NvFlowQueryTime* cpuTime);

/**
 * Steps the simulation dt forward in time.
 *
 * @param[in] grid The Flow grid to update.
 * @param[in] context The Flow context to perform the update.
 * @param[in] dt The time step, typically in seconds.
 */
NV_FLOW_API void NvFlowGridUpdate(NvFlowGrid* grid, NvFlowContext* context, float dt);

/**
 * Gets a view of the grid data for rendering.
 *
 * @param[in] grid The Flow grid to extract a view from.
 * @param[in] context The Flow context used to simulate the grid.
 *
 * @return The grid view for passing to rendering.
 */
NV_FLOW_API NvFlowGridView* NvFlowGridGetGridView(NvFlowGrid* grid, NvFlowContext* context);

/**
 * Not fully supported yet. Allows the application to request the grid move to a new location.
 *
 * @param[in] grid The Flow grid to move.
 * @param[in] targetLocation The location the center of the grid should make a best effort attempt to reach.
 */
NV_FLOW_API void NvFlowGridSetTargetLocation(NvFlowGrid* grid, NvFlowFloat3 targetLocation);

/**
 * Sets grid simulation parameters, persistent over multiple grid updates.
 *
 * @param[in] grid The Flow grid to set parameters on.
 * @param[in] params The new parameter values.
 */
NV_FLOW_API void NvFlowGridSetParams(NvFlowGrid* grid, const NvFlowGridParams* params);

/**
 * Adds one or more emit events to be applied with the next grid update.
 *
 * @param[in] grid The Flow grid to apply the emit events.
 * @param[in] shapes Array of shape data referenced by emit params.
 * @param[in] numShapes Number of shapes in the array.
 * @param[in] params Array of emit event parameters.
 * @param[in] numParams Number of emit events in the array.
 */
NV_FLOW_API void NvFlowGridEmit(NvFlowGrid* grid, const NvFlowShapeDesc* shapes, NvFlowUint numShapes, const NvFlowGridEmitParams* params, NvFlowUint numParams);

///@}
// -------------------------- NvFlowVolumeRender -------------------------------
///@defgroup NvFlowVolumeRender
///@{

//! A 1D texture mapping temperature and density to color
struct NvFlowColorMap;

//! Description required to create a color map
struct NvFlowColorMapDesc
{
	NvFlowUint resolution;		//!< Dimension of 1D texture used to store color map
};

//! Required information for writing to a CPU mapped color map
struct NvFlowColorMapData
{
	NvFlowFloat4* data;			//! Red, green, blue, alpha values
	NvFlowUint dim;				//! Number of float4 elements in mapped array
};

/**
 * Allows the application to request a default color map description from Flow.
 *
 * @param[out] desc The description for Flow to fill out.
 */
NV_FLOW_API void NvFlowColorMapDescDefaults(NvFlowColorMapDesc* desc);

/**
 * Creates a Flow color map, data uninitialized.
 *
 * @param[in] context The Flow context for 1D Texture allocation.
 * @param[in] desc Description for memory allocation.
 *
 * @return The created Flow color map.
 */
NV_FLOW_API NvFlowColorMap* NvFlowCreateColorMap(NvFlowContext* context, const NvFlowColorMapDesc* desc);

/**
 * Releases a Flow color map.
 *
 * @param[in] colorMap The Flow color map to be released.
 */
NV_FLOW_API void NvFlowReleaseColorMap(NvFlowColorMap* colorMap);

/**
 * Maps a color map for CPU write access.
 *
 * @param[in] colorMap The Flow color map object to map.
 * @param[in] context The Flow context used to create the Flow color map.
 *
 * @return The information needed to properly write to the mapped color map.
 */
NV_FLOW_API NvFlowColorMapData NvFlowColorMapMap(NvFlowColorMap* colorMap, NvFlowContext* context);

/**
 * Unmaps a color map from CPU write access, uploads updated color map to GPU.
 *
 * @param[in] colorMap The Flow color map to unmap.
 * @param[in] context The Flow context used to create the color map.
 */
NV_FLOW_API void NvFlowColorMapUnmap(NvFlowColorMap* colorMap, NvFlowContext* context);

//! Downsample options for offscreen ray march
enum NvFlowVolumeRenderDownsample
{
	eNvFlowVolumeRenderDownsampleNone = 0,
	eNvFlowVolumeRenderDownsample2x2 = 1
};

//! Multiple resolution options for offscreen ray march
enum NvFlowMultiResRayMarch
{
	eNvFlowMultiResRayMarchDisabled = 0,
	eNvFlowMultiResRayMarch2x2 = 1,
	eNvFlowMultiResRayMarch4x4 = 2,
	eNvFlowMultiResRayMarch8x8 = 3,
	eNvFlowMultiResRayMarch16x16 = 4,
};

//! Description needed to a create a Flow volume render object
struct NvFlowVolumeRenderDesc
{
	NvFlowGridView* view;								//!< Grid view for allocation purposes.
};

//! Rendering viewport
struct NvFlowVolumeRenderViewport
{
	float topLeftX;
	float topLeftY;
	float width;
	float height;
};

//! Parameters for VRWorks multires rendering
struct NvFlowVolumeRenderMultiResParams
{
	bool enabled;							//!< If true, app render target is assumed multiRes
	float centerWidth;						//!< Width of central viewport, ranging 0.01..1, where 1 is full orignal viewport width
	float centerHeight;						//!< Height of central viewport, ranging 0.01..1, where 1 is full orignal viewport height
	float centerX;							//!< X location of central viewport, ranging 0..1, where 0.5 is the center of the screen
	float centerY;							//!< Y location of central viewport, ranging 0..1, where 0.5 is the center of the screen
	float densityScaleX[3];					//!< Pixel density scale factors: how much the linear pixel density is scaled within each column (1.0 = full density)
	float densityScaleY[3];					//!< Pixel density scale factors: how much the linear pixel density is scaled within each row (1.0 = full density)
	NvFlowVolumeRenderViewport viewport;	//!< Single viewport representing the entire region to composite against
	float nonMultiResWidth;					//!< The render target width if multires was disabled
	float nonMultiResHeight;				//!< The render target height if multires was disabled
};

//! Parameters for VRWorks lens matched shading rendering
struct NvFlowVolumeRenderLMSParams
{
	bool enabled;							//!< If true, app render target is assumed lens matched shading
	float warpLeft;							
	float warpRight;
	float warpUp;
	float warpDown;
	float sizeLeft;
	float sizeRight;
	float sizeUp;
	float sizeDown;
	NvFlowVolumeRenderViewport viewport;	//!< Single viewport representing the entire region to composite against
	float nonLMSWidth;						//!< The render target width if lens matched shading was disabled
	float nonLMSHeight;						//!< The render target height if lens matched shading was disabled
};

//! Render modes
enum NvFlowVolumeRenderMode
{
	eNvFlowVolumeRenderMode_densityColormap = 0,
	eNvFlowVolumeRenderMode_velocityColormap = 1,
	eNvFlowVolumeRenderMode_densityRainbow = 2,
	eNvFlowVolumeRenderMode_velocityRainbow = 3,
	eNvFlowVolumeRenderMode_densityDebug = 4,
	eNvFlowVolumeRenderMode_velocityDebug = 5,
	eNvFlowVolumeRenderMode_densityRaw = 6,
	eNvFlowVolumeRenderMode_velocityRaw = 7
};

//! Parameters for Flow grid rendering
struct NvFlowVolumeRenderParams
{
	NvFlowFloat4x4 projectionMatrix;			//!< Projection matrix, row major
	NvFlowFloat4x4 viewMatrix;					//!< View matrix, row major
	NvFlowFloat4x4 modelMatrix;					//!< Model matrix, row major

	NvFlowDepthStencilView* depthStencilView;	//!< Depth stencil view for depth testing with ray march
	NvFlowRenderTargetView* renderTargetView;	//!< Render target view to composite ray marched result against

	float alphaScale;							//!< Global alpha scale for adjust net opacity without color map changes
	NvFlowUint renderMode;						//!< Render mode, see NvFlowVolumeRenderMode
	float colorMapMinX;							//!< Minimum value on the x channel (typically temperature), maps to colorMap u = 0.0
	float colorMapMaxX;							//!< Maximum value on the x channel (typically temperature), maps to colorMap u = 1.0

	NvFlowColorMap* colorMap;					//!< ColorMap to convert temperature to color

	bool debugMode;								//!< If true, wireframe visualization is rendered

	NvFlowVolumeRenderDownsample downsampleFactor;	//!< Controls size of ray marching render target relative to app render target.
	float screenPercentage;							//!< If 1.0, render at full ray march resolution, can be dynamically reduced toward 0.0 to ray march at a lower resolution
	NvFlowMultiResRayMarch multiResRayMarch;		//!< Coarsest downsample for multiple resolution ray march
	float multiResSamplingScale;					//!< 1.0 by default, increase for finer screen XY minimum sampling rate

	bool smoothColorUpsample;						//!< If true, color upsample will do extra work to remove jaggies around depth discontinuities

	NvFlowVolumeRenderMultiResParams multiRes;			//!< Multires parameters

	NvFlowVolumeRenderLMSParams lensMatchedShading;		//!< Lens matched shading parameters
};

/**
 * Allows the application to request default volume render parameters from Flow.
 *
 * @param[out] params The parameters for Flow to fill out.
 */
NV_FLOW_API void NvFlowVolumeRenderParamsDefaults(NvFlowVolumeRenderParams* params);

//! Parameters for Flow grid lighting
struct NvFlowVolumeLightingParams
{
	NvFlowUint renderMode;						//!< Render mode, see NvFlowVolumeRenderMode
	float colorMapMinX;							//!< Minimum value on the x channel (typically temperature), maps to colorMap u = 0.0
	float colorMapMaxX;							//!< Maximum value on the x channel (typically temperature), maps to colorMap u = 1.0

	NvFlowColorMap* colorMap;					//!< ColorMap to convert temperature to color
};

//! A Flow grid volume renderer
struct NvFlowVolumeRender;

/**
 * Creates a Flow volume render object.
 *
 * @param[in] context The Flow context for GPU resource allocation.
 * @param[in] desc Description for memory allocation.
 *
 * @return The created Flow volume render object.
 */
NV_FLOW_API NvFlowVolumeRender* NvFlowCreateVolumeRender(NvFlowContext* context, const NvFlowVolumeRenderDesc* desc);

/**
 * Releases a Flow volume render object.
 *
 * @param[in] volumeRender The Flow volume render object to be released.
 */
NV_FLOW_API void NvFlowReleaseVolumeRender(NvFlowVolumeRender* volumeRender);

/**
* Lights a Flow grid view to produce another grid view that can be ray marched raw.
*
* @param[in] volumeRender The Flow volume render object to perform the lighting.
* @param[in] context The Flow context that created the Flow volume render object.
* @param[in] colorMap The colorMap to use with colorMap render modes.
* @param[in] gridView The grid view to ray march.
* @param[in] params Parameters for lighting.
*
* @return The lit grid view.
*/
NV_FLOW_API NvFlowGridView* NvFlowVolumeRenderLightGridView(NvFlowVolumeRender* volumeRender, NvFlowContext* context, NvFlowGridView* gridView, const NvFlowVolumeLightingParams* params);

/**
 * Renders a Flow grid view.
 *
 * @param[in] volumeRender The Flow volume render object to perform the rendering.
 * @param[in] context The Flow context that created the Flow volume render object.
 * @param[in] gridView The grid view to ray march.
 * @param[in] params Parameters for rendering.
 */
NV_FLOW_API void NvFlowVolumeRenderGridView(NvFlowVolumeRender* volumeRender, NvFlowContext* context, NvFlowGridView* gridView, const NvFlowVolumeRenderParams* params);

/**
 * Renders a Flow 3D texture.
 *
 * @param[in] volumeRender The Flow volume render object to perform the rendering.
 * @param[in] context The Flow context that created the Flow volume render object.
 * @param[in] density The 3D texture to ray march.
 * @param[in] params Parameters for rendering.
 */
NV_FLOW_API void NvFlowVolumeRenderTexture3D(NvFlowVolumeRender* volumeRender, NvFlowContext* context, NvFlowTexture3D* density, const NvFlowVolumeRenderParams* params);

///@}
// -------------------------- NvFlowSDFGenerator -------------------------------
///@defgroup NvFlowSDFGenerator
///@{

//! A signed distance field generator
struct NvFlowSDFGen;

//! Description required for creating a Flow signed distance field generator
struct NvFlowSDFGenDesc
{
	NvFlowDim resolution;		//!< Resolution of 3D texture storing signed distance field
};

//! Simple mesh description
struct NvFlowSDFGenMeshParams
{
	NvFlowUint numVertices;			//!< Numbers of vertices in triangle mesh
	float* positions;				//!< Array of positions, stored in x, y, z order
	NvFlowUint positionStride;		//!< The distance between the beginning of one position to the beginning of the next position in array, in bytes
	float* normals;					//!< Array of normals, stored in nx, ny, nz order
	NvFlowUint normalStride;		//!< The distance between the beginning of one normal to the beginning of the next normal in array, in bytes

	NvFlowUint numIndices;			//!< Numbers of indices in triangle mesh
	NvFlowUint* indices;			//!< Array of indices

	NvFlowFloat4x4 modelMatrix;		//!< transforms from model space to SDF NDC space

	NvFlowDepthStencilView* depthStencilView;	//!< Depth stencil view to restore after voxelize work, lighter than Flow context push/pop
	NvFlowRenderTargetView* renderTargetView;	//!< Render target view to restore after voxelize work, lighter than Flow context push/pop
};

/**
 * Creates a Flow signed distance field generator.
 *
 * @param[in] context The Flow context for GPU resource allocation.
 * @param[in] desc Description for memory allocation.
 *
 * @return The created Flow signed distance field generator.
 */
NV_FLOW_API NvFlowSDFGen* NvFlowCreateSDFGen(NvFlowContext* context, const NvFlowSDFGenDesc* desc);

/**
 * Releases a Flow signed distance field generator.
 *
 * @param[in] sdfGen The Flow signed distance field generator to be released.
 */
NV_FLOW_API void NvFlowReleaseSDFGen(NvFlowSDFGen* sdfGen);

/**
 * Clears previous voxelization.
 *
 * @param[in] sdfGen The Flow signed distance field generator to test.
 * @param[in] context The Flow context that created sdfGen.
 */
NV_FLOW_API void NvFlowSDFGenReset(NvFlowSDFGen* sdfGen, NvFlowContext* context);

/**
 * Voxelizes triangle mesh.
 *
 * @param[in] sdfGen The Flow signed distance field generator to perform voxelization.
 * @param[in] context The Flow context that created sdfGen.
 * @param[in] params Parameters, including triangle mesh data.
 */
NV_FLOW_API void NvFlowSDFGenVoxelize(NvFlowSDFGen* sdfGen, NvFlowContext* context, const NvFlowSDFGenMeshParams* params);

/**
 * Generates signed distance field from latest voxelization.
 *
 * @param[in] sdfGen The Flow signed distance field generator to update.
 * @param[in] context The Flow context that created sdfGen.
 */
NV_FLOW_API void NvFlowSDFGenUpdate(NvFlowSDFGen* sdfGen, NvFlowContext* context);

/**
 * Provides access to signed distance field 3D Texture.
 *
 * @param[in] sdfGen The Flow signed distance field generator.
 * @param[in] context The Flow context that created sdfGen.
 *
 * @return The 3D texture storing the latest signed distance field.
 */
NV_FLOW_API NvFlowTexture3D* NvFlowSDFGenShape(NvFlowSDFGen* sdfGen, NvFlowContext* context);

///@}
// -------------------------- Multiple GPU Support -------------------------------
///@defgroup NvFlowDevice
///@{

//! A device exclusively for NvFlow simulation
struct NvFlowDevice;

//! Description required for creating a Flow device
struct NvFlowDeviceDesc
{
	bool autoSelectDevice;			//!< if true, NvFlow tries to identify best compute device
	NvFlowUint adapterIdx;			//!< preferred device index
};

//! Flow device status to allow app to throttle maximum queued work
struct NvFlowDeviceStatus
{
	NvFlowUint framesInFlight;		//!< Number of flushes that have not completed work on the GPU
};

/**
 * Allows the application to request a default Flow device description from Flow.
 *
 * @param[out] desc The description for Flow to fill out.
 */
NV_FLOW_API void NvFlowDeviceDescDefaults(NvFlowDeviceDesc* desc);

/**
 * Checks if a GPU is available that is not being used for application graphics work.
 *
 * @param[in] renderContext A Flow context that maps to the application graphics GPU.
 *
 * @return Returns true if dedicated GPU is available.
 */
NV_FLOW_API bool NvFlowDedicatedDeviceAvailable(NvFlowContext* renderContext);

/**
 * Creates a Flow compute device.
 *
 * @param[in] renderContext A Flow context that maps to the application graphics GPU.
 * @param[in] desc Description that controls what GPU is selected.
 *
 * @return The created Flow compute device.
 */
NV_FLOW_API NvFlowDevice* NvFlowCreateDevice(NvFlowContext* renderContext, const NvFlowDeviceDesc* desc);

/**
 * Releases a Flow compute device.
 *
 * @param[in] device The Flow compute device to be released.
 */
NV_FLOW_API void NvFlowReleaseDevice(NvFlowDevice* device);

/**
 * Creates a Flow context that uses a Flow compute device.
 *
 * @param[in] device The Flow compute device to create the context against.
 *
 * @return The created Flow context.
 */
NV_FLOW_API NvFlowContext* NvFlowDeviceCreateContext(NvFlowDevice* device);

/**
 * Updates a Flow context that uses a Flow compute device.
 *
 * @param[in] device The Flow compute device the context was created against.
 * @param[in] context The Flow context update.
 * @param[out] status The status of device for management of work queued.
 */
NV_FLOW_API void NvFlowDeviceUpdateContext(NvFlowDevice* device, NvFlowContext* context, NvFlowDeviceStatus* status);

/**
 * Flushes all submitted work to the Flow device. Needed for reliable execution with a compute only device.
 *
 * @param[in] device The Flow compute device to flush.
 */
NV_FLOW_API void NvFlowDeviceFlush(NvFlowDevice* device);

///@}
///@defgroup NvFlowGridProxy
///@{

//! A proxy for a grid simulated on one device to render on a different device, currently limited to Windows 10 for multi-GPU support.
struct NvFlowGridProxy;

//! Description required to create a grid proxy.
struct NvFlowGridProxyDesc
{
	bool singleGPUMode;				//!< if true, proxy is transparent, simply passes pushed grid view
};

/**
 * Creates a Flow grid proxy.
 *
 * @param[in] gridContext The Flow context that simulates the Flow grid.
 * @param[in] grid The Flow grid to create a proxy for.
 * @param[in] desc Description describing kind of proxy to create.
 *
 * @return The created Flow grid proxy.
 */
NV_FLOW_API NvFlowGridProxy* NvFlowCreateGridProxy(NvFlowContext* gridContext, NvFlowGrid* grid, const NvFlowGridProxyDesc* desc);

/**
 * Releases a Flow grid proxy.
 *
 * @param[in] proxy The Flow grid proxy to be released.
 */
NV_FLOW_API void NvFlowReleaseGridProxy(NvFlowGridProxy* proxy);

/**
 * Pushes simulation results to the proxy, should be updated after each simulation update.
 *
 * @param[in] proxy The Flow grid proxy to be updated.
 * @param[in] gridContext The Flow context that simulated the grid.
 * @param[in] grid The Flow grid with updated simulation results.
 */
NV_FLOW_API void NvFlowGridProxyPush(NvFlowGridProxy* proxy, NvFlowContext* gridContext, NvFlowGrid* grid);

/**
 * Helps simulation results move faster between GPUs, should be called before each render.
 *
 * @param[in] proxy The Flow grid proxy to be updated.
 * @param[in] gridContext The Flow context that simulated the grid.
 */
NV_FLOW_API void NvFlowGridProxyFlush(NvFlowGridProxy* proxy, NvFlowContext* gridContext);

/**
 * Returns the latest grid view available on the render GPU.
 *
 * @param[in] proxy The Flow grid proxy supplying the grid view.
 * @param[in] renderContext The Flow context that will render the grid view.
 *
 * @return The latest grid view available from the proxy.
 */
NV_FLOW_API NvFlowGridView* NvFlowGridProxyGetGridView(NvFlowGridProxy* proxy, NvFlowContext* renderContext);

///@}
// -------------------------- NvFlowGridExport -------------------------------
///@defgroup NvFlowGridExport
///@{

//! Object to expose read access Flow grid simulation data
struct NvFlowGridExport;

struct NvFlowGridExportDesc
{
	NvFlowGridView* gridView;
};

struct NvFlowGridExportViewShaderParams
{
	NvFlowUint4 blockDim;
	NvFlowUint4 blockDimBits;
	NvFlowFloat4 blockDimInv;
	NvFlowUint4 linearBlockDim;
	NvFlowUint4 linearBlockOffset;
	NvFlowFloat4 dimInv;
	NvFlowFloat4 vdim;
	NvFlowFloat4 vdimInv;
	NvFlowUint4 poolGridDim;
	NvFlowUint4 gridDim;
	NvFlowUint4 isVTR;
};

struct NvFlowGridExportView
{
	NvFlowResource* data;
	NvFlowResource* blockTable;
	NvFlowResource* blockList;

	NvFlowGridExportViewShaderParams shaderParams;

	NvFlowUint numBlocks;
	NvFlowUint maxBlocks;

	NvFlowFloat4x4 modelMatrix;
};

NV_FLOW_API NvFlowGridExport* NvFlowCreateGridExport(NvFlowContext* context, const NvFlowGridExportDesc* desc);

NV_FLOW_API void NvFlowReleaseGridExport(NvFlowGridExport* gridExport);

NV_FLOW_API void NvFlowGridExportUpdate(NvFlowGridExport* gridExport, NvFlowContext* context, NvFlowGridView* gridView, NvFlowGridChannel channel);

NV_FLOW_API void NvFlowGridExportGetView(NvFlowGridExport* gridExport, NvFlowContext* context, NvFlowGridExportView* view, NvFlowGridChannel channel);

///@}
// -------------------------- NvFlowGridImport -------------------------------
///@defgroup NvFlowGridImport
///@{

//! Object to expose write access to Flow grid simulation data
struct NvFlowGridImport;

struct NvFlowGridImportDesc
{
	NvFlowGridView* gridView;
};

struct NvFlowGridImportViewShaderParams
{
	NvFlowUint4 blockDim;
	NvFlowUint4 blockDimBits;
	NvFlowUint4 poolGridDim;
	NvFlowUint4 gridDim;
	NvFlowUint4 isVTR;
};

struct NvFlowGridImportView
{
	NvFlowResourceRW* dataRW;
	NvFlowResource* blockTable;
	NvFlowResource* blockList;

	NvFlowGridImportViewShaderParams shaderParams;

	NvFlowUint numBlocks;
	NvFlowUint maxBlocks;

	NvFlowFloat4x4 modelMatrix;
};

NV_FLOW_API NvFlowGridImport* NvFlowCreateGridImport(NvFlowContext* context, const NvFlowGridImportDesc* desc);

NV_FLOW_API void NvFlowReleaseGridImport(NvFlowGridImport* gridImport);

NV_FLOW_API void NvFlowGridImportGetView(NvFlowGridImport* gridImport, NvFlowContext* context, NvFlowGridImportView* view, NvFlowGridView* gridView, NvFlowGridChannel channel);

NV_FLOW_API void NvFlowGridImportUpdate(NvFlowGridImport* gridImport, NvFlowContext* context, NvFlowGridChannel channel);

NV_FLOW_API NvFlowGridView* NvFlowGridImportGetGridView(NvFlowGridImport* gridImport, NvFlowContext* context);

///@}