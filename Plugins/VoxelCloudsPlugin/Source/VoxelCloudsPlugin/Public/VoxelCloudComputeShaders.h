#pragma once

#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "ShaderParameterStruct.h"

#define VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE FVector3f

class FVoxelCloudExistenceComputeShader final : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FVoxelCloudExistenceComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FVoxelCloudExistenceComputeShader, FGlobalShader);
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, VOXELCLOUDSPLUGIN_API)
		SHADER_PARAMETER(FUintVector3, VoxelGridSize)
		SHADER_PARAMETER(FVector3f, Offset)
		SHADER_PARAMETER(float, VoxelSize)
		SHADER_PARAMETER(float, TotalTime)
		SHADER_PARAMETER(float, CloudinessThreshold)
		SHADER_PARAMETER(float, NoiseScale)
		SHADER_PARAMETER(float, Roundedness)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, OutputBuffer)
	END_SHADER_PARAMETER_STRUCT()
};

class FVoxelCloudsMarchingCubesComputeShader final : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FVoxelCloudsMarchingCubesComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FVoxelCloudsMarchingCubesComputeShader, FGlobalShader);
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, VOXELCLOUDSPLUGIN_API)
		SHADER_PARAMETER(FUintVector3, VoxelGridSize)
		SHADER_PARAMETER(FVector3f, Offset)
		SHADER_PARAMETER(float, VoxelSize)
		SHADER_PARAMETER(float, TotalTime)
		SHADER_PARAMETER(float, CloudinessThreshold)
		SHADER_PARAMETER(float, NoiseScale)
		SHADER_PARAMETER(float, Roundedness)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, VoxelData)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FVector3f>, TriangleBuffer)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWByteAddressBuffer, CounterBuffer)
	END_SHADER_PARAMETER_STRUCT()
};

class FVoxelCloudComputeShaders
{
public:
	// Dispatches this shader. Can be called from any thread
	static void Dispatch(const FVoxelCloudExistenceComputeShader::FParameters& Parameters, TFunction<void(TArray<VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE>)> AsyncCallback);

private:
	static void DispatchGameThread(const FVoxelCloudExistenceComputeShader::FParameters& Parameters, TFunction<void(TArray<VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE>)> AsyncCallback);
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FVoxelCloudExistenceComputeShader::FParameters& Parameters, TFunction<void(TArray<VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE>)> AsyncCallback);
};
