#include "VoxelCloudComputeShaders.h"

#include "RenderGraph.h"
#include "RHIGPUReadback.h"

DECLARE_STATS_GROUP(TEXT("VoxelClouds"), STATGROUP_VoxelClouds, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Voxel Cloud Compute Shader"), STAT_VoxelCloudComputeShader, STATGROUP_VoxelClouds);

IMPLEMENT_GLOBAL_SHADER(FVoxelCloudsMarchingCubesComputeShader, "/VoxelCloudsPluginShaders/MarchingCubesComputeShader.usf", "MainComputeShader", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FVoxelCloudExistenceComputeShader, "/VoxelCloudsPluginShaders/MyVoxelExistenceComputeShader.usf", "MainComputeShader", SF_Compute);

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
		///////////////////////////////
		//SETUP SECOND COMPUTE SHADER//
		///////////////////////////////

		TShaderMapRef<FVoxelCloudsMarchingCubesComputeShader> ComputeShaderB(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		if (!ComputeShaderA.IsValid())
		{
			check(0);
			return;
		}

		//INPUT PARAMETERS
		FVoxelCloudsMarchingCubesComputeShader::FParameters* PassParametersB = GraphBuilder.AllocParameters<FVoxelCloudsMarchingCubesComputeShader::FParameters>();
		PassParametersB->Offset = PassParametersA->Offset;
		PassParametersB->Roundedness = PassParametersA->Roundedness;
		PassParametersB->CloudinessThreshold = PassParametersA->CloudinessThreshold;
		PassParametersB->NoiseScale = PassParametersA->NoiseScale;
		PassParametersB->TotalTime = PassParametersA->TotalTime;
		PassParametersB->VoxelData = PassParametersA->OutputBuffer;
		PassParametersB->VoxelSize = PassParametersA->VoxelSize;
		PassParametersB->VoxelGridSize = PassParametersA->VoxelGridSize;

		//OUTPUTS
		FRDGBufferRef TriangleBuffer = GraphBuilder.CreateBuffer(
			FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), NumVoxels * 5 * 3),
			TEXT("TriangleBuffer"));

		PassParametersB->TriangleBuffer = GraphBuilder.CreateUAV(TriangleBuffer);

		FRDGBufferDesc CounterDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), 1); // Only 1 uint needed for counter
		//CounterDesc.Usage = EBufferUsageFlags::ByteAddressBuffer | EBufferUsageFlags::UnorderedAccess;
		FRDGBufferRef CounterBuffer = GraphBuilder.CreateBuffer(CounterDesc, TEXT("TriangleCounter"));
		PassParametersB->CounterBuffer = GraphBuilder.CreateUAV(CounterBuffer, PF_R32_UINT);
		
		AddClearUAVPass(GraphBuilder, PassParametersB->CounterBuffer, 0);

		GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Parameters.VoxelGridSize), FIntVector(8, 8, 8));

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("RunVoxelCloudComputeShaderB"),
			ComputeShaderB,
			PassParametersB,
			GroupCount);
		

		// Set up GPU readback
		FRHIGPUBufferReadback* VertexReadback = new FRHIGPUBufferReadback(TEXT("VertexReadback"));
		AddEnqueueCopyPass(GraphBuilder, VertexReadback, TriangleBuffer, 0);
		FRHIGPUBufferReadback* CounterReadback = new FRHIGPUBufferReadback(TEXT("CounterReadback"));
		AddEnqueueCopyPass(GraphBuilder, CounterReadback, CounterBuffer, 0);


		// Now we know how many elements were appended
		auto RunnerFunc = [VertexReadback, CounterReadback, AsyncCallback](auto&& RunnerFunc) -> void {
			if (VertexReadback->IsReady() && CounterReadback->IsReady())
			{
				uint32 Count = *static_cast<uint32*>(CounterReadback->Lock(0));
				CounterReadback->Unlock();
								
				const VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE* DataPtr = static_cast<VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE*>(VertexReadback->Lock(0));
				TArray<VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE> OutputData;
				OutputData.Append(DataPtr, Count);
				VertexReadback->Unlock();

				delete VertexReadback;
				delete CounterReadback;

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
