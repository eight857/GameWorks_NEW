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
#include "NvFlowShader.h"

// -------------------------- NvFlowGrid -------------------------------
///@defgroup NvFlowGrid
///@{

//! A Flow dynamic grid
struct NvFlowGrid;

//! Interface to expose read access Flow grid simulation data
struct NvFlowGridExport;

//! Grid texture channel, four components per channel
enum NvFlowGridTextureChannel
{
	eNvFlowGridTextureChannelVelocity = 0,
	eNvFlowGridTextureChannelDensity = 1,
	eNvFlowGridTextureChannelDensityCoarse = 2,

	eNvFlowGridTextureChannelCount
};

//! Enumeration used to describe density texture channel resolution relative to velocity resolution
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
	float coarseResidentScaleFactor;	//!< Allows relative increase of resident scale for coarse sparse textures

	bool enableVTR;						//!< Enable use of volume tiled resources, if supported
	bool lowLatencyMapping;				//!< Faster mapping updates, more mapping overhead but less prediction required
};

/**
 * Allows the application to request a default grid description from Flow.
 *
 * @param[out] desc The description for Flow to fill out.
 */
NV_FLOW_API void NvFlowGridDescDefaults(NvFlowGridDesc* desc);

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

//! Description required to reset a NvFlowGrid
struct NvFlowGridResetDesc
{
	NvFlowFloat3 initialLocation;		//!< Initial location of axis aligned bounding box
	NvFlowFloat3 halfSize;				//!< Initial half size of axis aligned bounding box
};

/**
 * Allows the application to request a default grid reset description from Flow.
 *
 * @param[out] desc The description for Flow to fill out.
 */
NV_FLOW_API void NvFlowGridResetDescDefaults(NvFlowGridResetDesc* desc);

/**
 * Submits a request to reset a Flow grid, preserving memory allocations
 *
 * @param[in] context The Flow context to reset the Flow grid
 * @param[in] desc The Flow grid description.
 */
NV_FLOW_API void NvFlowGridReset(NvFlowGrid* grid, const NvFlowGridResetDesc* desc);

/**
 * Not fully supported yet. Allows the application to request the grid move to a new location.
 *
 * @param[in] grid The Flow grid to move.
 * @param[in] targetLocation The location the center of the grid should make a best effort attempt to reach.
 */
NV_FLOW_API void NvFlowGridSetTargetLocation(NvFlowGrid* grid, NvFlowFloat3 targetLocation);

//! Flags to control grid debug visualization
enum NvFlowGridDebugVisFlags
{
	eNvFlowGridDebugVisDisabled = 0x00,		//!< No debug visualization
	eNvFlowGridDebugVisBlocks = 0x01,		//!< Simulation block visualization, no overhead
	eNvFlowGridDebugVisEmitBounds = 0x02,	//!< Emitter bounds visualization, adds overhead
	eNvFlowGridDebugVisShapesSimple = 0x04, //!< Visualize sphere/capsule/box shapes, adds overhead

	eNvFlowGridDebugVisCount
};

//! Parameters controlling grid behavior
struct NvFlowGridParams
{
	NvFlowFloat3 gravity;					//!< Gravity vector for use by buoyancy

	bool pressureLegacyMode;				//!< If true, run older less accurate pressure solver

	bool bigEffectMode;						//!< Tweaks block allocation for better big effect behavior
	float bigEffectPredictTime;				//!< Time constant to tune big effect prediction

	NvFlowGridDebugVisFlags debugVisFlags;	//!< Flags to control what debug visualization information is generated
};

/**
 * Allows the application to request default grid parameters from Flow.
 *
 * @param[out] params The parameters for Flow to fill out.
 */
NV_FLOW_API void NvFlowGridParamsDefaults(NvFlowGridParams* params);

/**
 * Sets grid simulation parameters, persistent over multiple grid updates.
 *
 * @param[in] grid The Flow grid to set parameters on.
 * @param[in] params The new parameter values.
 */
NV_FLOW_API void NvFlowGridSetParams(NvFlowGrid* grid, const NvFlowGridParams* params);

//! Description of feature support on the queried Flow context GPU.
struct NvFlowSupport
{
	bool supportsVTR;		//!< True if volume tiled resources are supported
};

/**
 * Queries support for features that depend on hardware/OS.
 *
 * @param[in] grid The Flow grid to query for support.
 * @param[in] context The Flow context the grid was created against.
 * @param[out] support Description of what is supported.
 *
 * @return Returns eNvFlowSuccess if information is available.
 */
NV_FLOW_API NvFlowResult NvFlowGridQuerySupport(NvFlowGrid* grid, NvFlowContext* context, NvFlowSupport* support);

//! CPU/GPU timing info
struct NvFlowQueryTime
{
	float simulation;
};

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
 * Queries simulation GPU memory usage.
 *
 * @param[in] grid The Flow grid to query for timing.
 * @param[out] numBytes GPU memory allocated in bytes.
 */
NV_FLOW_API void NvFlowGridGPUMemUsage(NvFlowGrid* grid, NvFlowUint64* numBytes);

/**
 * Steps the simulation dt forward in time.
 *
 * @param[in] grid The Flow grid to update.
 * @param[in] context The Flow context to perform the update.
 * @param[in] dt The time step, typically in seconds.
 */
NV_FLOW_API void NvFlowGridUpdate(NvFlowGrid* grid, NvFlowContext* context, float dt);

/**
* Get read interface to the grid simulation results
*
* @param[in] context The context the grid was created with.
* @param[in] grid The Flow grid to query for timing.
*
* @return Returns gridExport interface
*/
NV_FLOW_API NvFlowGridExport* NvFlowGridGetGridExport(NvFlowContext* context, NvFlowGrid* grid);

///@}
// -------------------------- NvFlowGridMaterial -------------------------------
///@defgroup NvFlowGridMaterial
///@{

//! Handle provided by grid to reference materials
struct NvFlowGridMaterialHandle
{
	NvFlowGrid* grid;		//!< The grid that created this material handle
	NvFlowUint64 uid;
};

//! Grid component IDs
enum NvFlowGridComponent
{
	eNvFlowGridComponentVelocity = 0,
	eNvFlowGridComponentSmoke = 1,
	eNvFlowGridComponentTemperature = 2,
	eNvFlowGridComponentFuel = 3,

	eNvFlowGridNumComponents = 4
};

//! Grid material per component parameters
struct NvFlowGridMaterialPerComponent
{
	float damping;						//!< Higher values reduce component value faster (exponential decay curve)
	float fade;							//!< Fade component value rate in units / sec
	float macCormackBlendFactor;		//!< Higher values make a sharper appearance, but with more artifacts
	float macCormackBlendThreshold;		//!< Minimum absolute value to apply MacCormack correction. Increasing can improve performance.
	float allocWeight;					//!< Relative importance of component value for allocation, 0.0 means not important
	float allocThreshold;				//!< Minimum component value magnitude that is considered relevant
};

//! Grid material parameters
struct NvFlowGridMaterialParams
{
	NvFlowGridMaterialPerComponent velocity;	//!< Velocity component parameters
	NvFlowGridMaterialPerComponent smoke;		//!< Smoke component parameters
	NvFlowGridMaterialPerComponent temperature;	//!< Temperature component parameters
	NvFlowGridMaterialPerComponent fuel;		//!< Fuel component parameters

	float vorticityStrength;					//!< Higher values increase rotation, reduce laminar flow
	float vorticityVelocityMask;				//!< 0.f means zero velocity magnitude influence on vorticity

	float ignitionTemp;							//!< Minimum temperature for combustion
	float burnPerTemp;							//!< Burn amount per unit temperature above ignitionTemp
	float fuelPerBurn;							//!< Fuel consumed per unit burn
	float tempPerBurn;							//!< Temperature increase per unit burn
	float smokePerBurn;							//!< Smoke increase per unit burn
	float divergencePerBurn;					//!< Expansion per unit burn
	float buoyancyPerTemp;						//!< Buoyant force per unit temperature
	float coolingRate;							//!< Cooling rate, exponential
};

/**
* Allows the application to request default grid material parameters from Flow.
*
* @param[out] params The parameters for Flow to fill out.
*/
NV_FLOW_API void NvFlowGridMaterialParamsDefaults(NvFlowGridMaterialParams* params);

/**
* Gets a handle to the default grid material
*
* @param[in] grid The Flow grid to set parameters on.
*/
NV_FLOW_API NvFlowGridMaterialHandle NvFlowGridGetDefaultMaterial(NvFlowGrid* grid);

/**
* Creates new grid material, initializes to params
*
* @param[in] grid The Flow grid to set parameters on.
* @param[in] params The new parameter values.
*/
NV_FLOW_API NvFlowGridMaterialHandle NvFlowGridCreateMaterial(NvFlowGrid* grid, const NvFlowGridMaterialParams* params);

/**
* Release grid material
*
* @param[in] grid The Flow grid to set parameters on.
* @param[in] material Handle to material to release.
*/
NV_FLOW_API void NvFlowGridReleaseMaterial(NvFlowGrid* grid, NvFlowGridMaterialHandle material);

/**
* Sets material parameters, persistent over multiple grid updates.
*
* @param[in] grid The Flow grid to set parameters on.
* @param[in] material Handle to material to update.
* @param[in] params The new parameter values.
*/
NV_FLOW_API void NvFlowGridSetMaterialParams(NvFlowGrid* grid, NvFlowGridMaterialHandle material, const NvFlowGridMaterialParams* params);

///@}
// -------------------------- NvFlowShape -------------------------------
///@defgroup NvFlowShape
///@{

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

///@}
// -------------------------- NvFlowGridEmit -------------------------------
///@defgroup NvFlowGridEmit
///@{

//! Emitter modes
enum NvFlowGridEmitMode
{
	eNvFlowGridEmitModeDefault = 0,					//!< Emitter will influence velocity and density channels, optionally allocate based on bounds
	eNvFlowGridEmitModeDisableVelocity = 0x01,		//!< Flag to disable emitter interaction with velocity field
	eNvFlowGridEmitModeDisableDensity = 0x02,		//!< Flag to disable emitter interaction with density field
	eNvFlowGridEmitModeDisableAlloc = 0x04,			//!< Flag to disable emitter bound allocation
	eNvFlowGridEmitModeAllocShape = 0x08,			//!< Emitter will allocate using shape to drive allocation instead of only bounds

	eNvFlowGridEmitModeAllocShapeOnly = 0x0F,		//!< Flags to configure for shape aware allocation only
};

//! Parameters for both emission and collision
struct NvFlowGridEmitParams
{
	NvFlowUint shapeRangeOffset;					//!< Start of shape range, offset in number of Shapes
	NvFlowUint shapeRangeSize;						//!< Size of shape range, in number of Shapes
	NvFlowShapeType shapeType;						//!< Type of shape in the set
	float shapeDistScale;							//!< Scale to apply to SDF value

	NvFlowFloat4x4 bounds;							//!< Transform from emitter ndc to world space
	NvFlowFloat4x4 localToWorld;					//!< Transform from shape local space to world space
	NvFlowFloat3 centerOfMass; 						//!< Center of mass in emitter local coordinate space

	float deltaTime;								//!< DeltaTime used to compute impulse

	NvFlowGridMaterialHandle material;				//!< Material for this emitter
	NvFlowUint emitMode;							//!< Emitter behavior, based on NvFlowGridEmitMode, 0u is default
	NvFlowUint numSubSteps;							//!< Numbers of interations to perform on cell value

	NvFlowFloat3 allocationScale;					//!< Higher values cause more blocks to allocate around emitter; 0.f means no allocation, 1.f is default
	float allocationPredict;						//!< Higher values cause extra allocation based on linear velocity and predict velocity
	NvFlowFloat3 predictVelocity;					//!< Velocity used only for predict
	float predictVelocityWeight;					//!< Blend weight between linearVelocity and predictVelocity

	float minActiveDist;							//!< Minimum distance value for active emission
	float maxActiveDist;							//!< Maximum distance value for active emission
	float minEdgeDist;								//!< Distance from minActiveDist to 1.0 emitter opacity
	float maxEdgeDist;								//!< Distance before maxActiveDist to 0.0 emitter opacity
	float slipThickness;							//!< Thickness of slip boundary region
	float slipFactor;								//!< 0.0 => no slip, fully damped; 1.0 => full slip

	NvFlowFloat3 velocityLinear;					//!< Linear velocity, in world units, emitter direction
	NvFlowFloat3 velocityAngular;					//!< Angular velocity, in world units, emitter direction
	NvFlowFloat3 velocityCoupleRate;				//!< Rate of correction to target, inf means instantaneous

	float smoke;									//!< Target smoke
	float smokeCoupleRate;							//!< Rate of correction to target, inf means instantaneous

	float temperature;								//!< Target temperature
	float temperatureCoupleRate;					//!< Rate of correction to target, inf means instantaneous

	float fuel;										//!< Target fuel
	float fuelCoupleRate;							//!< Rate of correction to target, inf means instantaneous
	float fuelReleaseTemp;							//!< Minimum temperature to release fuelRelease additional fuel
	float fuelRelease;								//!< Fuel released when temperature exceeds release temperature
};

/**
* Allows the application to request default emit parameters from Flow.
*
* @param[out] params The parameters for Flow to fill out.
*/
NV_FLOW_API void NvFlowGridEmitParamsDefaults(NvFlowGridEmitParams* params);

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
// -------------------------- NvFlowGridEmitCustomAlloc -------------------------------
///@defgroup NvFlowGridEmitCustomAlloc
///@{

//! Necessary parameters/resources for custom grid block allocation
struct NvFlowGridEmitCustomAllocParams
{
	NvFlowResourceRW* maskResourceRW;	//!< Integer mask, write 1u where allocation is desired

	NvFlowDim maskDim;					//!< Mask dimensions

	NvFlowFloat3 gridLocation;			//!< Location of grid's axis aligned bounding box
	NvFlowFloat3 gridHalfSize;			//!< Half size of grid's axis aligned bounding box

	NvFlowGridMaterialHandle material;	//!< Grid material
};

typedef void(*NvFlowGridEmitCustomAllocFunc)(void* userdata, const NvFlowGridEmitCustomAllocParams* params);

/**
 * Sets custom allocation callback.
 *
 * @param[in] grid The Flow grid to use the callback.
 * @param[in] func The callback function.
 * @param[in] userdata Pointer to provide to the callback function during execution.
 */
NV_FLOW_API void NvFlowGridEmitCustomRegisterAllocFunc(NvFlowGrid* grid, NvFlowGridEmitCustomAllocFunc func, void* userdata);

//! Handle for requesting per layer emitter data
struct NvFlowGridEmitCustomEmitParams
{
	NvFlowGrid* grid;			//!< The grid associated with this callback
	NvFlowUint numLayers;		//!< The number of layers to write to
	void* flowInternal;			//!< For Flow internal use, do not modify
};

//! Necessary parameters/resources for custom emit operations
struct NvFlowGridEmitCustomEmitLayerParams
{
	NvFlowResourceRW* dataRW[2u];				//!< Read/Write 3D textures for channel data
	NvFlowResource* blockTable;					//!< Table to map virtual blocks to real blocks
	NvFlowResource* blockList;					//!< List of active blocks

	NvFlowShaderPointParams shaderParams;		//!< Parameters used in GPU side operations

	NvFlowUint numBlocks;						//!< Number of active blocks
	NvFlowUint maxBlocks;						//!< Maximum possible active blocks

	NvFlowFloat3 gridLocation;					//!< Location of grid's axis aligned bounding box
	NvFlowFloat3 gridHalfSize;					//!< Half size of grid's axis aligned bounding box

	NvFlowGridMaterialHandle material;			//!< Grid material
};

typedef void(*NvFlowGridEmitCustomEmitFunc)(void* userdata, NvFlowUint* dataFrontIdx, const NvFlowGridEmitCustomEmitParams* params);

/**
 * Sets custom emit callback for given simulation channel.
 *
 * @param[in] grid The Flow grid to use the callback.
 * @param[in] channel The simulation channel for this callback.
 * @param[in] func The callback function.
 * @param[in] userdata Pointer to provide to the callback function during execution.
 */
NV_FLOW_API void NvFlowGridEmitCustomRegisterEmitFunc(NvFlowGrid* grid, NvFlowGridTextureChannel channel, NvFlowGridEmitCustomEmitFunc func, void* userdata);

/**
* Get per layer custom emit parameters, should only be called inside the custom emit callback
*
* @param[in] emitParams The custom emit parameters
* @param[in] layerIdx The layerIdx to fetch, should be least than emitParams->numLayers
* @param[out] emitLayerParams Pointer to write parameters to.
*/
NV_FLOW_API void NvFlowGridEmitCustomGetLayerParams(const NvFlowGridEmitCustomEmitParams* emitParams, NvFlowUint layerIdx, NvFlowGridEmitCustomEmitLayerParams* emitLayerParams);

///@}
// -------------------------- NvFlowGridExportImport -------------------------------
///@defgroup NvFlowGridExport
///@{

//! Description of a single exported layer
struct NvFlowGridExportImportLayerMapping
{
	NvFlowGridMaterialHandle material;

	NvFlowResource* blockTable;
	NvFlowResource* blockList;

	NvFlowUint numBlocks;
};

//! Description applying to all exported layers
struct NvFlowGridExportImportLayeredMapping
{
	NvFlowShaderLinearParams shaderParams;
	NvFlowUint maxBlocks;

	NvFlowUint2* layeredBlockListCPU;
	NvFlowUint layeredNumBlocks;

	NvFlowFloat4x4 modelMatrix;
};

///@}
// -------------------------- NvFlowGridExport -------------------------------
///@defgroup NvFlowGridExport
///@{

//! Texture channel export handle
struct NvFlowGridExportHandle
{
	NvFlowGridExport* gridExport;
	NvFlowGridTextureChannel channel;
	NvFlowUint numLayerViews;
};

//! Description of a single exported layer
struct NvFlowGridExportLayerView
{
	NvFlowResource* data;
	NvFlowGridExportImportLayerMapping mapping;
};

//! Description applying to all exported layers
struct NvFlowGridExportLayeredView
{
	NvFlowGridExportImportLayeredMapping mapping;
};

//! Data to visualize simple shape
struct NvFlowGridExportSimpleShape
{
	NvFlowFloat4x4 localToWorld;
	NvFlowShapeDesc shapeDesc;
};

//! Debug vis data
struct NvFlowGridExportDebugVisView
{
	NvFlowGridDebugVisFlags debugVisFlags;

	NvFlowFloat4x4* bounds;
	NvFlowUint numBounds;
	NvFlowGridExportSimpleShape* spheres;
	NvFlowUint numSpheres;
	NvFlowGridExportSimpleShape* capsules;
	NvFlowUint numCapsules;
	NvFlowGridExportSimpleShape* boxes;
	NvFlowUint numBoxes;
};

NV_FLOW_API NvFlowGridExportHandle NvFlowGridExportGetHandle(NvFlowGridExport* gridExport, NvFlowContext* context, NvFlowGridTextureChannel channel);

NV_FLOW_API void NvFlowGridExportGetLayerView(NvFlowGridExportHandle handle, NvFlowUint layerIdx, NvFlowGridExportLayerView* layerView);

NV_FLOW_API void NvFlowGridExportGetLayeredView(NvFlowGridExportHandle handle, NvFlowGridExportLayeredView* layeredView);

NV_FLOW_API void NvFlowGridExportGetDebugVisView(NvFlowGridExport* gridExport, NvFlowGridExportDebugVisView* view);

///@}
// -------------------------- NvFlowGridImport -------------------------------
///@defgroup NvFlowGridImport
///@{

//! Object to expose write access to Flow grid simulation data
struct NvFlowGridImport;

//! Description required to create GridImport
struct NvFlowGridImportDesc
{
	NvFlowGridExport* gridExport;
};

enum NvFlowGridImportMode
{
	eNvFlowGridImportModePoint = 0,
	eNvFlowGridImportModeLinear = 1
};

//! Parameters for grabbing import view
struct NvFlowGridImportParams
{
	NvFlowGridExport* gridExport;
	NvFlowGridTextureChannel channel;
	NvFlowGridImportMode importMode;
};

//! Texture channel handle
struct NvFlowGridImportHandle
{
	NvFlowGridImport* gridImport;
	NvFlowGridTextureChannel channel;
	NvFlowUint numLayerViews;
};

//! Description of a single imported layer
struct NvFlowGridImportLayerView
{
	NvFlowResourceRW* dataRW;			//!< This always should be written
	NvFlowResourceRW* blockTableRW;		//!< If StateCPU path is used, this needs to be written, else is nullptr
	NvFlowResourceRW* blockListRW;		//!< If StateCPU path is used, this needs to be written, else is nullptr
	NvFlowGridExportImportLayerMapping mapping;
};

//! Description applying to all imported layers
struct NvFlowGridImportLayeredView
{
	NvFlowGridExportImportLayeredMapping mapping;
};

NV_FLOW_API NvFlowGridImport* NvFlowCreateGridImport(NvFlowContext* context, const NvFlowGridImportDesc* desc);

NV_FLOW_API void NvFlowReleaseGridImport(NvFlowGridImport* gridImport);

NV_FLOW_API NvFlowGridImportHandle NvFlowGridImportGetHandle(NvFlowGridImport* gridImport, NvFlowContext* context, const NvFlowGridImportParams* params);

NV_FLOW_API void NvFlowGridImportGetLayerView(NvFlowGridImportHandle handle, NvFlowUint layerIdx, NvFlowGridImportLayerView* layerView);

NV_FLOW_API void NvFlowGridImportGetLayeredView(NvFlowGridImportHandle handle, NvFlowGridImportLayeredView* layeredView);

NV_FLOW_API void NvFlowGridImportReleaseChannel(NvFlowGridImport* gridImport, NvFlowContext* context, NvFlowGridTextureChannel channel);

NV_FLOW_API NvFlowGridExport* NvFlowGridImportGetGridExport(NvFlowGridImport* gridImport, NvFlowContext* context);

//! Object to hold captured CPU export state
struct NvFlowGridImportStateCPU;

//! Parameters for grabbing import view
struct NvFlowGridImportStateCPUParams
{
	NvFlowGridImportStateCPU* stateCPU;
	NvFlowGridTextureChannel channel;
	NvFlowGridImportMode importMode;
};

NV_FLOW_API NvFlowGridImportStateCPU* NvFlowCreateGridImportStateCPU(NvFlowGridImport* gridImport);

NV_FLOW_API void NvFlowReleaseGridImportStateCPU(NvFlowGridImportStateCPU* stateCPU);

NV_FLOW_API void NvFlowGridImportUpdateStateCPU(NvFlowGridImportStateCPU* stateCPU, NvFlowContext* context, NvFlowGridExport* gridExport);

NV_FLOW_API NvFlowGridImportHandle NvFlowGridImportStateCPUGetHandle(NvFlowGridImport* gridImport, NvFlowContext* context, const NvFlowGridImportStateCPUParams* params);

///@}
// -------------------------- NvFlowRenderMaterial -------------------------------
///@defgroup NvFlowRenderMaterial
///@{

//! A pool of render materials
struct NvFlowRenderMaterialPool;

struct NvFlowRenderMaterialPoolDesc
{
	NvFlowUint colorMapResolution;		//!< Dimension of 1D texture used to store color map, 64 is a good default
};

/**
* Creates a Flow render material pool object.
*
* @param[in] context The Flow context for GPU resource allocation.
* @param[in] desc Description for memory allocation.
*
* @return The created Flow volume render object.
*/
NV_FLOW_API NvFlowRenderMaterialPool* NvFlowCreateRenderMaterialPool(NvFlowContext* context, const NvFlowRenderMaterialPoolDesc* desc);

/**
* Releases a Flow volume render object.
*
* @param[in] pool The Flow volume render object to be released.
*/
NV_FLOW_API void NvFlowReleaseRenderMaterialPool(NvFlowRenderMaterialPool* pool);

//! A handle to a volume render material
struct NvFlowRenderMaterialHandle
{
	NvFlowRenderMaterialPool* pool;			//!< The pool that created this material
	NvFlowUint64 uid;
};

//! Render modes
enum NvFlowVolumeRenderMode
{
	eNvFlowVolumeRenderMode_colormap = 0,
	eNvFlowVolumeRenderMode_raw = 1,
	eNvFlowVolumeRenderMode_rainbow = 2,
	eNvFlowVolumeRenderMode_debug = 3,

	eNvFlowVolumeRenderModeCount
};

//! Per material parameters for Flow grid rendering
struct NvFlowRenderMaterialParams
{
	NvFlowGridMaterialHandle material;			//!< Grid material to align these parameters with

	float alphaScale;							//!< Global alpha scale for adjust net opacity without color map changes, applied after saturate(alpha)
	float additiveFactor;						//!< 1.0 makes material blend fully additive

	NvFlowFloat4 colorMapCompMask;				//!< Component mask for colormap, control what channel drives color map X axis;
	NvFlowFloat4 alphaCompMask;					//!< Component mask to control which channel(s) modulation the alpha
	NvFlowFloat4 intensityCompMask;				//!< Component mask to control which channel(s) modulates the intensity

	float colorMapMinX;							//!< Minimum value on the x channel (typically temperature), maps to colorMap u = 0.0
	float colorMapMaxX;							//!< Maximum value on the x channel (typically temperature), maps to colorMap u = 1.0
	float alphaBias;							//!< Offsets alpha before saturate(alpha)
	float intensityBias;						//!< Offsets intensity before modulating color
};

/**
* Allows the application to request default volume render material parameters from Flow.
*
* @param[out] params The parameters for Flow to fill out.
*/
NV_FLOW_API void NvFlowRenderMaterialParamsDefaults(NvFlowRenderMaterialParams* params);

/**
* Get the default render material.
*
* @param[in] pool The pool to create/own the material.
*
* @return A handle to the material.
*/
NV_FLOW_API NvFlowRenderMaterialHandle NvFlowGetDefaultRenderMaterial(NvFlowRenderMaterialPool* pool);

/**
* Create a render material.
*
* @param[in] context The context to use for GPU resource creation.
* @param[in] pool The pool to create/own the material.
* @param[in] params Material parameters.
*
* @return A handle to the material.
*/
NV_FLOW_API NvFlowRenderMaterialHandle NvFlowCreateRenderMaterial(NvFlowContext* context, NvFlowRenderMaterialPool* pool, const NvFlowRenderMaterialParams* params);

/**
* Release a render material.
*
* @param[in] handle Handle to the material to release.
*/
NV_FLOW_API void NvFlowReleaseRenderMaterial(NvFlowRenderMaterialHandle handle);

/**
* Update a render material.
*
* @param[in] volumeRender The Flow volume render object.
* @param[in] handle Handle to the material to update.
* @param[in] params Material parameter.
*/
NV_FLOW_API void NvFlowRenderMaterialUpdate(NvFlowRenderMaterialHandle handle, const NvFlowRenderMaterialParams* params);

//! Required information for writing to a CPU mapped color map
struct NvFlowColorMapData
{
	NvFlowFloat4* data;			//! Red, green, blue, alpha values
	NvFlowUint dim;				//! Number of float4 elements in mapped array
};

/**
* Map the color map associated with the material.
*
* @param[in] context The context to use for mapping.
* @param[in] handle Handle to the material to map.
*/
NV_FLOW_API NvFlowColorMapData NvFlowRenderMaterialColorMap(NvFlowContext* context, NvFlowRenderMaterialHandle handle);

/**
* Unmap the color map associated with the material.
*
* @param[in] context The context to perform unmap.
* @param[in] handle Handle to the material to unmap.
*/
NV_FLOW_API void NvFlowRenderMaterialColorUnmap(NvFlowContext* context, NvFlowRenderMaterialHandle handle);


///@}
// -------------------------- NvFlowVolumeRender -------------------------------
///@defgroup NvFlowVolumeRender
///@{

//! A Flow grid volume renderer
struct NvFlowVolumeRender;

//! Description needed to a create a Flow volume render object
struct NvFlowVolumeRenderDesc
{
	NvFlowGridExport* gridExport;				//!< Export interface
};

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

//! Parameters for Flow grid rendering
struct NvFlowVolumeRenderParams
{
	NvFlowFloat4x4 projectionMatrix;			//!< Projection matrix, row major
	NvFlowFloat4x4 viewMatrix;					//!< View matrix, row major
	NvFlowFloat4x4 modelMatrix;					//!< Model matrix, row major

	NvFlowDepthStencilView* depthStencilView;	//!< Depth stencil view for depth testing with ray march
	NvFlowRenderTargetView* renderTargetView;	//!< Render target view to composite ray marched result against

	NvFlowRenderMaterialPool* materialPool;		//!< Pool of materials to look for matches to GridMaterials

	NvFlowVolumeRenderMode renderMode;			//!< Render mode, see NvFlowVolumeRenderMode
	NvFlowGridTextureChannel renderChannel;		//!< GridView channel to render

	bool debugMode;								//!< If true, wireframe visualization is rendered

	NvFlowVolumeRenderDownsample downsampleFactor;	//!< Controls size of ray marching render target relative to app render target.
	float screenPercentage;							//!< If 1.0, render at full ray march resolution, can be dynamically reduced toward 0.0 to ray march at a lower resolution
	NvFlowMultiResRayMarch multiResRayMarch;		//!< Coarsest downsample for multiple resolution ray march
	float multiResSamplingScale;					//!< 1.0 by default, increase for finer screen XY minimum sampling rate

	bool smoothColorUpsample;						//!< If true, color upsample will do extra work to remove jaggies around depth discontinuities

	bool estimateDepth;								//!< If true, generate nominal depth, and write to scene depth buffer
	bool estimateDepthDebugMode;					//!< If true, visualize depth estimate

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
	NvFlowRenderMaterialPool* materialPool;		//!< Pool of materials to look for matches to GridMaterials

	NvFlowVolumeRenderMode renderMode;			//!< Render mode, see NvFlowVolumeRenderMode
	NvFlowGridTextureChannel renderChannel;		//!< GridView channel to render
};

//! Parameters for Flow scene depth capture
struct NvFlowVolumeRenderDepthCaptureParams
{
	NvFlowFloat4x4 projectionMatrix;			//!< Projection matrix, row major
	NvFlowFloat4x4 viewMatrix;					//!< View matrix, row major

	NvFlowDepthStencilView* depthStencilView;	//!< Depth stencil view for depth testing with ray march
};

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
NV_FLOW_API NvFlowGridExport* NvFlowVolumeRenderLightGridExport(NvFlowVolumeRender* volumeRender, NvFlowContext* context, NvFlowGridExport* gridExport, const NvFlowVolumeLightingParams* params);

/**
 * Renders a Flow grid view.
 *
 * @param[in] volumeRender The Flow volume render object to perform the rendering.
 * @param[in] context The Flow context that created the Flow volume render object.
 * @param[in] gridView The grid view to ray march.
 * @param[in] params Parameters for rendering.
 */
NV_FLOW_API void NvFlowVolumeRenderGridExport(NvFlowVolumeRender* volumeRender, NvFlowContext* context, NvFlowGridExport* gridExport, const NvFlowVolumeRenderParams* params);

/**
* Captures the scene depth for later use.
*
* @param[in] volumeRender The Flow volume render object to perform the rendering.
* @param[in] context The Flow context that created the Flow volume render object.
* @param[in] params Parameters for scene depth capture.
*/
NV_FLOW_API void NvFlowVolumeRenderCaptureDepth(NvFlowVolumeRender* volumeRender, NvFlowContext* context, const NvFlowVolumeRenderDepthCaptureParams* params);

/**
* Renders a Flow grid export depth.
*
* @param[in] volumeRender The Flow volume render object to perform the rendering.
* @param[in] context The Flow context that created the Flow volume render object.
* @param[in] gridView The grid view to ray march.
* @param[in] params Parameters for rendering.
*/
NV_FLOW_API void NvFlowVolumeRenderGridExportRenderDepth(NvFlowVolumeRender* volumeRender, NvFlowContext* context, NvFlowGridExport* gridExport, const NvFlowVolumeRenderParams* params);

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
// -------------------------- NvFlowVolumeShadow -------------------------------
///@defgroup NvFlowVolumeShadow
///@{

//! Object to generate shadows from gridView
struct NvFlowVolumeShadow;

struct NvFlowVolumeShadowDesc
{
	NvFlowGridExport* gridExport;

	NvFlowUint mapWidth;
	NvFlowUint mapHeight;
	NvFlowUint mapDepth;

	float minResidentScale;				//!< Minimum (and initial) fraction of virtual cells to allocate memory for
	float maxResidentScale;				//!< Maximum fraction of virtual cells to allocate memory for
};

struct NvFlowVolumeShadowParams
{
	NvFlowFloat4x4 projectionMatrix;			//!< Projection matrix, row major
	NvFlowFloat4x4 viewMatrix;					//!< View matrix, row major

	NvFlowRenderMaterialPool* materialPool;		//!< Pool of materials to look for matches to GridMaterials

	NvFlowVolumeRenderMode renderMode;			//!< Render mode, see NvFlowVolumeRenderMode
	NvFlowGridTextureChannel renderChannel;		//!< GridView channel to render

	float intensityScale;						//!< Shadow intensity scale
	float minIntensity;							//!< Minimum shadow intensity

	NvFlowFloat4 shadowBlendCompMask;			//!< Component mask to control which channel(s) modulate the shadow blending
	float shadowBlendBias;						//!< Bias on shadow blend factor
};

struct NvFlowVolumeShadowDebugRenderParams
{
	NvFlowRenderTargetView* renderTargetView;

	NvFlowFloat4x4 projectionMatrix;			//!< Render target projection matrix, row major
	NvFlowFloat4x4 viewMatrix;					//!< Render target view matrix, row major
};

struct NvFlowVolumeShadowStats
{
	NvFlowUint shadowColumnsActive;
	NvFlowUint shadowBlocksActive;
	NvFlowUint shadowCellsActive;
};

NV_FLOW_API NvFlowVolumeShadow* NvFlowCreateVolumeShadow(NvFlowContext* context, const NvFlowVolumeShadowDesc* desc);

NV_FLOW_API void NvFlowReleaseVolumeShadow(NvFlowVolumeShadow* volumeShadow);

NV_FLOW_API void NvFlowVolumeShadowUpdate(NvFlowVolumeShadow* volumeShadow, NvFlowContext* context, NvFlowGridExport* gridView, const NvFlowVolumeShadowParams* params);

NV_FLOW_API NvFlowGridExport* NvFlowVolumeShadowGetGridExport(NvFlowVolumeShadow* volumeShadow, NvFlowContext* context);

NV_FLOW_API void NvFlowVolumeShadowDebugRender(NvFlowVolumeShadow* volumeShadow, NvFlowContext* context, const NvFlowVolumeShadowDebugRenderParams* params);

NV_FLOW_API void NvFlowVolumeShadowGetStats(NvFlowVolumeShadow* volumeShadow, NvFlowVolumeShadowStats* stats);

///@}
// -------------------------- NvFlowCrossSection -------------------------------
///@defgroup NvFlowCrossSection
///@{

//! Object to visualize cross section from gridView
struct NvFlowCrossSection;

struct NvFlowCrossSectionDesc
{
	NvFlowGridExport* gridExport;
};

struct NvFlowCrossSectionParams
{
	NvFlowGridExport* gridExport;				//!< gridExport used for final rendering
	NvFlowGridExport* gridExportDebugVis;		//!< gridExport direct from simulation

	NvFlowFloat4x4 projectionMatrix;			//!< Projection matrix, row major
	NvFlowFloat4x4 viewMatrix;					//!< View matrix, row major

	NvFlowDepthStencilView* depthStencilView;	//!< Depth stencil view for depth testing with ray march
	NvFlowRenderTargetView* renderTargetView;	//!< Render target view to composite ray marched result against

	NvFlowRenderMaterialPool* materialPool;		//!< Pool of materials to look for matches to GridMaterials

	NvFlowVolumeRenderMode renderMode;			//!< Render mode, see NvFlowVolumeRenderMode
	NvFlowGridTextureChannel renderChannel;		//!< GridView channel to render

	NvFlowUint crossSectionAxis;				//!< Cross section to visualize, 0 to 2 range
	NvFlowFloat3 crossSectionPosition;			//!< Offset in grid NDC for view
	float crossSectionScale;					//!< Scale on cross section to allow zooming

	float intensityScale;						//!< scales the visualization intensity

	bool pointFilter;							//!< If true, point filter so the cells are easy to see

	bool velocityVectors;						//!< If true, overlay geometric velocity vectors
	float velocityScale;						//!< Scale to adjust vector length as a function of velocity
	float vectorLengthScale;					//!< Controls maximum velocity vector line length

	bool outlineCells;							//!< Draw lines around cell boundaries

	bool fullscreen;							//!< If true, covers entire viewport, if false, top right corner

	NvFlowFloat4 lineColor;						//!< Color to use for any lines drawn
	NvFlowFloat4 backgroundColor;				//!< Background color
	NvFlowFloat4 cellColor;						//!< Color for cell outline
};

NV_FLOW_API void NvFlowCrossSectionParamsDefaults(NvFlowCrossSectionParams* params);

NV_FLOW_API NvFlowCrossSection* NvFlowCreateCrossSection(NvFlowContext* context, const NvFlowCrossSectionDesc* desc);

NV_FLOW_API void NvFlowReleaseCrossSection(NvFlowCrossSection* crossSection);

NV_FLOW_API void NvFlowCrossSectionRender(NvFlowCrossSection* crossSection, NvFlowContext* context, const NvFlowCrossSectionParams* params);

///@}
// -------------------------- NvFlowGridProxy -------------------------------
///@defgroup NvFlowGridProxy
///@{

//! A proxy for a grid simulated on one device to render on a different device, currently limited to Windows 10 for multi-GPU support.
struct NvFlowGridProxy;

//! Proxy types
enum NvFlowGridProxyType
{
	eNvFlowGridProxyTypePassThrough = 0,
	eNvFlowGridProxyTypeMultiGPU = 1,
	eNvFlowGridProxyTypeInterQueue = 2,
};

//! Parameters need to create a grid proxy
struct NvFlowGridProxyDesc
{
	NvFlowContext* gridContext;			//!< FlowContext used to simulate grid
	NvFlowContext* renderContext;		//!< FlowContext used to render grid
	NvFlowContext* gridCopyContext;		//!< FlowContext with copy capability on gridContext device
	NvFlowContext* renderCopyContext;	//!< FlowContext with copy capability on renderContext device

	NvFlowGridExport* gridExport;		//!< GridExport to base allocation on

	NvFlowGridProxyType proxyType;		//!< GridProxy type to create
};

//! Parameters need to create a multi-GPU proxy
struct NvFlowGridProxyFlushParams
{
	NvFlowContext* gridContext;
	NvFlowContext* gridCopyContext;
	NvFlowContext* renderCopyContext;
};

/**
* Creates a passthrough Flow grid proxy, for improved single vs multi-GPU compatibility
*
* @param[in] desc Description required to create grid proxy
*
* @return The created Flow grid proxy.
*/
NV_FLOW_API NvFlowGridProxy* NvFlowCreateGridProxy(const NvFlowGridProxyDesc* desc);

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
* @param[in] gridExport The Flow gridExport with updated simulation results.
* @param[in] params Parameters needed to flush the data.
*/
NV_FLOW_API void NvFlowGridProxyPush(NvFlowGridProxy* proxy, NvFlowGridExport* gridExport, const NvFlowGridProxyFlushParams* params);

/**
* Helps simulation results move faster between GPUs, should be called before each render.
*
* @param[in] proxy The Flow grid proxy to be updated.
* @param[in] params Parameters needed to flush the data.
*/
NV_FLOW_API void NvFlowGridProxyFlush(NvFlowGridProxy* proxy, const NvFlowGridProxyFlushParams* params);

/**
* Returns the latest grid view available on the render GPU.
*
* @param[in] proxy The Flow grid proxy supplying the grid view.
* @param[in] renderContext The Flow context that will render the grid view.
*
* @return The latest grid view available from the proxy.
*/
NV_FLOW_API NvFlowGridExport* NvFlowGridProxyGetGridExport(NvFlowGridProxy* proxy, NvFlowContext* renderContext);

///@}
// -------------------------- NvFlowDevice -------------------------------
///@defgroup NvFlowDevice
///@{

//! A device exclusively for NvFlow simulation
struct NvFlowDevice;

//! Device Type
enum NvFlowDeviceMode
{
	eNvFlowDeviceModeProxy = 0,		//!< Exposes renderContext device
	eNvFlowDeviceModeUnique = 1,	//!< Generates unique device, not matching renderContext
};

//! Description required for creating a Flow device
struct NvFlowDeviceDesc
{
	NvFlowDeviceMode mode;			//!< Type of device to create
	bool autoSelectDevice;			//!< if true, NvFlow tries to identify best compute device
	NvFlowUint adapterIdx;			//!< preferred device index
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
* Checks if a GPU can support a dedicated queue
*
* @param[in] renderContext A Flow context that maps to the application graphics GPU.
*
* @return Returns true if dedicated device queue is available.
*/
NV_FLOW_API bool NvFlowDedicatedDeviceQueueAvailable(NvFlowContext* renderContext);

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

//! A device queue created through an NvFlowDevice
struct NvFlowDeviceQueue;

//! Types of queues
enum NvFlowDeviceQueueType
{
	eNvFlowDeviceQueueTypeGraphics = 0,
	eNvFlowDeviceQueueTypeCompute = 1,
	eNvFlowDeviceQueueTypeCopy = 2
};

//! Description required for creating a Flow device queue
struct NvFlowDeviceQueueDesc
{
	NvFlowDeviceQueueType queueType;
	bool lowLatency;
};

//! Flow device queue status to allow app to throttle maximum queued work
struct NvFlowDeviceQueueStatus
{
	NvFlowUint framesInFlight;			//!< Number of flushes that have not completed work on the GPU
	NvFlowUint64 lastFenceCompleted;	//!< The last fence completed on device queue
	NvFlowUint64 nextFenceValue;		//!< The fence value signaled after flush
};

/**
* Creates a Flow device queue.
*
* @param[in] renderContext A Flow context that maps to the application graphics GPU.
* @param[in] desc Description that controls kind of device queue to create.
*
* @return The created Flow device queue.
*/
NV_FLOW_API NvFlowDeviceQueue* NvFlowCreateDeviceQueue(NvFlowDevice* device, const NvFlowDeviceQueueDesc* desc);

/**
* Releases a Flow device queue.
*
* @param[in] deviceQueue The Flow device queue to be released.
*/
NV_FLOW_API void NvFlowReleaseDeviceQueue(NvFlowDeviceQueue* deviceQueue);

/**
* Creates a Flow context that uses a Flow device queue.
*
* @param[in] deviceQueue The Flow device queue to create the context against.
*
* @return The created Flow context.
*/
NV_FLOW_API NvFlowContext* NvFlowDeviceQueueCreateContext(NvFlowDeviceQueue* deviceQueue);

/**
* Updates a Flow context that uses a Flow device queue.
*
* @param[in] deviceQueue The Flow device queue the context was created against.
* @param[in] context The Flow context update.
*/
NV_FLOW_API void NvFlowDeviceQueueUpdateContext(NvFlowDeviceQueue* deviceQueue, NvFlowContext* context, NvFlowDeviceQueueStatus* status);

/**
* Flushes all submitted work to the Flow deviceQueue. Must be called to submit work to queue.
*
* @param[in] deviceQueue The Flow deviceQueue to flush.
* @param[in] context The Flow context to sync with the flush event
*/
NV_FLOW_API void NvFlowDeviceQueueFlush(NvFlowDeviceQueue* deviceQueue, NvFlowContext* context);

/**
* Flushes all submitted work to the Flow deviceQueue if the context requests a flush.
*
* @param[in] deviceQueue The Flow deviceQueue to conditionally flush.
* @param[in] context The Flow context to sync with the flush event
*/
NV_FLOW_API void NvFlowDeviceQueueConditionalFlush(NvFlowDeviceQueue* deviceQueue, NvFlowContext* context);

/**
* Blocks CPU until fenceValue is reached.
*
* @param[in] deviceQueue The Flow deviceQueue to flush.
* @param[in] context The Flow context to sync with the flush event
* @param[in] fenceValue The fence value to wait for.
*/
NV_FLOW_API void NvFlowDeviceQueueWaitOnFence(NvFlowDeviceQueue* deviceQueue, NvFlowContext* context, NvFlowUint64 fenceValue);

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
// -------------------------- NvFlowParticleSurface -------------------------------
///@defgroup NvFlowParticleSurface
///@{

//! A particle surface generator
struct NvFlowParticleSurface;

//! Description for creation
struct NvFlowParticleSurfaceDesc
{
	NvFlowFloat3 initialLocation;		//!< Initial location of axis aligned bounding box
	NvFlowFloat3 halfSize;				//!< Initial half size of axis aligned bounding box

	NvFlowDim virtualDim;				//!< Resolution of virtual address space inside of bounding box
	float residentScale;				//!< Fraction of virtual cells to allocate memory for

	NvFlowUint maxParticles;			//!< Maximum particle count for memory allocation
};

//! Particle data
struct NvFlowParticleSurfaceData
{
	const float* positions;
	NvFlowUint positionStride;
	NvFlowUint numParticles;
};

//! Parameters for update
struct NvFlowParticleSurfaceParams
{
	float surfaceThreshold;
	float smoothRadius;
	bool separableSmoothing;
};

//! Parameter for surface emission
struct NvFlowParticleSurfaceEmitParams
{
	float deltaTime;

	NvFlowFloat3 velocityLinear;					//!< Linear velocity, in world units, emitter direction
	NvFlowFloat3 velocityCoupleRate;				//!< Rate of correction to target, inf means instantaneous

	float smoke;									//!< Target smoke
	float smokeCoupleRate;							//!< Rate of correction to target, inf means instantaneous

	float temperature;								//!< Target temperature
	float temperatureCoupleRate;					//!< Rate of correction to target, inf means instantaneous

	float fuel;										//!< Target fuel
	float fuelCoupleRate;							//!< Rate of correction to target, inf means instantaneous
};

NV_FLOW_API NvFlowParticleSurface* NvFlowCreateParticleSurface(NvFlowContext* context, const NvFlowParticleSurfaceDesc* desc);

NV_FLOW_API void NvFlowReleaseParticleSurface(NvFlowParticleSurface* surface);

NV_FLOW_API void NvFlowParticleSurfaceUpdateParticles(NvFlowParticleSurface* surface, NvFlowContext* context, const NvFlowParticleSurfaceData* data);

NV_FLOW_API void NvFlowParticleSurfaceUpdateSurface(NvFlowParticleSurface* surface, NvFlowContext* context, const NvFlowParticleSurfaceParams* params);

NV_FLOW_API void NvFlowParticleSurfaceAllocFunc(NvFlowParticleSurface* surface, NvFlowContext* context, const NvFlowGridEmitCustomAllocParams* params);

NV_FLOW_API void NvFlowParticleSurfaceEmitVelocityFunc(NvFlowParticleSurface* surface, NvFlowContext* context, NvFlowUint* dataFrontIdx, const NvFlowGridEmitCustomEmitParams* params, const NvFlowParticleSurfaceEmitParams* emitParams);

NV_FLOW_API void NvFlowParticleSurfaceEmitDensityFunc(NvFlowParticleSurface* surface, NvFlowContext* context, NvFlowUint* dataFrontIdx, const NvFlowGridEmitCustomEmitParams* params, const NvFlowParticleSurfaceEmitParams* emitParams);

NV_FLOW_API NvFlowGridExport* NvFlowParticleSurfaceDebugGridExport(NvFlowParticleSurface* surface, NvFlowContext* context);

///@}