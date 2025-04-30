#include "VoxelCloudComputeShaders.h"

#include "RenderGraph.h"
#include "RHIGPUReadback.h"

DECLARE_STATS_GROUP(TEXT("VoxelClouds"), STATGROUP_VoxelClouds, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Voxel Cloud Compute Shader"), STAT_VoxelCloudComputeShader, STATGROUP_VoxelClouds);

void FVoxelCloudComputeShaders::Dispatch(const FVoxelCloudExistenceComputeShader::FParameters& Parameters, TFunction<void(TArray<VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE>)> AsyncCallback) {
	if (IsInRenderingThread()) {
		FVoxelCloudExistenceComputeShader::FParameters OutParams = Parameters;
		DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), OutParams, AsyncCallback);
	}else{
		DispatchGameThread(Parameters, AsyncCallback);
	}
}

void FVoxelCloudComputeShaders::DispatchGameThread(const FVoxelCloudExistenceComputeShader::FParameters& Parameters,
	TFunction<void(TArray<VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE>)> AsyncCallback) {
	ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
		[Parameters, AsyncCallback](FRHICommandListImmediate& RHICmdList)
		{
			FVoxelCloudExistenceComputeShader::FParameters OutParams = Parameters;
			DispatchRenderThread(RHICmdList, OutParams, AsyncCallback);
		});
}


void FVoxelCloudComputeShaders::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FVoxelCloudExistenceComputeShader::FParameters& Parameters, TFunction<void(TArray<VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE>)> AsyncCallback)
{
	FRDGBuilder GraphBuilder(RHICmdList);

	{
		SCOPE_CYCLE_COUNTER(STAT_VoxelCloudComputeShader);
		DECLARE_GPU_STAT(VoxelCloudComputeShader);
		RDG_EVENT_SCOPE(GraphBuilder, "VoxelCloudComputeShader");

		const uint32 NumVoxels = Parameters.VoxelGridSize.X * Parameters.VoxelGridSize.Y * Parameters.VoxelGridSize.Z;
		const uint32 NumPoints = (Parameters.VoxelGridSize.X + 1) * (Parameters.VoxelGridSize.Y + 1) * (Parameters.VoxelGridSize.Z + 1);
		TShaderMapRef<FVoxelCloudExistenceComputeShader> ComputeShaderA(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		if (!ComputeShaderA.IsValid())
		{
			check(0);
			return;
		}

		FVoxelCloudExistenceComputeShader::FParameters* PassParametersA = GraphBuilder.AllocParameters<FVoxelCloudExistenceComputeShader::FParameters>(&Parameters);


		FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(
			FRDGBufferDesc::CreateStructuredDesc(sizeof(float), NumPoints),
			TEXT("OutputBuffer"));

		PassParametersA->OutputBuffer = GraphBuilder.CreateUAV(OutputBuffer);

		FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Parameters.VoxelGridSize) + FIntVector(1,1,1), FIntVector(8, 8, 8));

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("RunVoxelCloudComputeShaderA"),
			ComputeShaderA,
			PassParametersA,
			GroupCount);
				

		// Set up GPU readback
		FRHIGPUBufferReadback* Readback = new FRHIGPUBufferReadback(TEXT("VoxelCloudReadback"));
		AddEnqueueCopyPass(GraphBuilder, Readback, OutputBuffer, 0);

		// Now we know how many elements were appended
		auto RunnerFunc = [Readback, AsyncCallback, NumPoints](auto&& RunnerFunc) -> void {
			if (Readback->IsReady())
			{
				const VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE* DataPtr = static_cast<VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE*>(Readback->Lock(0));
				TArray<VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE> OutputData;
				OutputData.Append(DataPtr, NumPoints);
				Readback->Unlock();

				delete Readback;

				AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutputData = MoveTemp(OutputData)]() {
					AsyncCallback(OutputData);
				});
			}
			else
			{
				AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
					RunnerFunc(RunnerFunc);
				});
			}
		};

		AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
			RunnerFunc(RunnerFunc);
			});
	}

	GraphBuilder.Execute();
}
