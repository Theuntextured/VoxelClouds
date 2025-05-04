#include "VoxelCloudSceneProxy.h"
#include "CloudRenderer.h"
#include "DynamicMeshBuilder.h"
#include "MaterialDomain.h"
#include "NullVertexBuffers.h"
#include "Materials/MaterialRenderProxy.h"

void FVoxelCloudIndexBuffer::SetData(const TArray<uint32>& InIndices)
{
	//IndexBufferRHI.SafeRelease();
	FRHICommandListBase& RHICmdList = FRHICommandListImmediate::Get();

	FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelCloudIndexBuffer"));
	const int32 DataSize = sizeof(uint32) * InIndices.Num();
	IndexBufferRHI = RHICmdList.CreateIndexBuffer(sizeof(uint32), DataSize, BUF_Volatile, CreateInfo);

	void* Data = RHICmdList.LockBuffer(IndexBufferRHI, 0, DataSize, RLM_WriteOnly);
	FMemory::Memcpy(Data, InIndices.GetData(), DataSize);
	RHICmdList.UnlockBuffer(IndexBufferRHI);

	ShaderResourceViewRHI = RHICmdList.CreateShaderResourceView(IndexBufferRHI, sizeof(uint32), PF_R32_UINT);
	//UnorderedAccessViewRHI = RHICmdList.CreateUnorderedAccessView(IndexBufferRHI, PF_R32_UINT);
}

void FVoxelCloudIndexBuffer::ReleaseResource()
{
	FIndexBuffer::ReleaseResource();

	ShaderResourceViewRHI.SafeRelease();
	//UnorderedAccessViewRHI.SafeRelease();
}

void FVoxelCloudVertexBuffer::SetData(const TArray<FVector3f>& InPositions)
{
	//VertexBufferRHI.SafeRelease();
	FRHICommandListBase& RHICmdList = FRHICommandListImmediate::Get();

	FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelCloudVertexBuffer"));
	const int32 DataSize = sizeof(FVector3f) * InPositions.Num();
	VertexBufferRHI = RHICmdList.CreateVertexBuffer(DataSize, BUF_Volatile | BUF_ShaderResource, CreateInfo);
	void* Data = RHICmdList.LockBuffer(VertexBufferRHI, 0, DataSize, RLM_WriteOnly);
	FMemory::Memcpy(Data, InPositions.GetData(), DataSize);
	RHICmdList.UnlockBuffer(VertexBufferRHI);
	
	ShaderResourceViewRHI = RHICmdList.CreateShaderResourceView(VertexBufferRHI, sizeof(float), PF_R32_FLOAT);
	//UnorderedAccessViewRHI = RHICmdList.CreateUnorderedAccessView(VertexBufferRHI, PF_R32G32B32F);
}

FVoxelCloudSceneProxy::FVoxelCloudSceneProxy(UCloudRendererComponent* Component): FPrimitiveSceneProxy(Component),
                                                                                  Vertices(Component->Vertices), Indices(Component->Indices)
{
	if (Component->GetMaterial(0) && Component->GetMaterial(0)->GetRenderProxy())
		MaterialProxy = Component->GetMaterial(0)->GetRenderProxy();
	else
		MaterialProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
}

void FVoxelCloudSceneProxy::CreateRenderThreadResources()
{
	IndexBuffer.InitResource(FRHICommandListImmediate::Get());
	VertexBuffer.InitResource(FRHICommandListImmediate::Get());
	IndexBuffer.SetData(Indices);
	VertexBuffer.SetData(Vertices);
	VertexFactory = new FVoxelCloudVertexFactory(GetScene().GetFeatureLevel());
	InitVertexFactoryData();
	
}

void FVoxelCloudSceneProxy::DestroyRenderThreadResources()
{
	IndexBuffer.ReleaseResource();
	VertexBuffer.ReleaseResource();
	if(VertexFactory)
	{
		VertexFactory->ReleaseResource();
		delete VertexFactory;
	}
}

void FVoxelCloudSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views,
                                                   const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

	FMaterialRenderProxy* MaterialProxyToSubmit = const_cast<FMaterialRenderProxy*>(MaterialProxy);
	if (bWireframe)
	{
		MaterialProxyToSubmit = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : NULL,
			FLinearColor(0, 0.5f, 1.f)
		);

		Collector.RegisterOneFrameMaterialProxy(MaterialProxyToSubmit);
	}
	
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
	{
		if (VisibilityMap & (1u << ViewIndex))
		{
#if 0
			FDynamicMeshBuilder MeshBuilder(Views[ViewIndex]->GetFeatureLevel());
			// Add each vertex
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(Gather Cloud Vertices);
				for (int32 i = 0; i < Vertices.Num(); ++i)
					MeshBuilder.AddVertex(FDynamicMeshVertex(Vertices[i]));
			}
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(Gather Cloud Triangles);
				MeshBuilder.AddTriangles(Indices);
			}
			{
                    
				TRACE_CPUPROFILER_EVENT_SCOPE(Get Cloud Mesh);
				// Draw
				MeshBuilder.GetMesh(
					GetLocalToWorld(),
					MaterialProxy,
					SDPG_World,
					false, false, false,
					Collector
				);
			}
			continue;
#endif
			FRHICommandListBase& RHICmdList = FRHICommandListImmediate::Get();
			FMeshBatch& Mesh = Collector.AllocateMesh();
			FMeshBatchElement& BatchElement = Mesh.Elements[0];

			BatchElement.IndexBuffer = &IndexBuffer;
			Mesh.bWireframe = bWireframe;
			Mesh.VertexFactory = VertexFactory;
			Mesh.MaterialRenderProxy = MaterialProxyToSubmit;

			bool bHasPrecomputedVolumetricLightmap;
			FMatrix PreviousLocalToWorld;
			int32 SingleCaptureIndex;
			bool bOutputVelocity;
			GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);
			//Alloate a temporary primitive uniform buffer, fill it with the data and set it in the batch element
			FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
			DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, bOutputVelocity);
			BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;
			BatchElement.PrimitiveIdMode = PrimID_DynamicPrimitiveShaderData;

			BatchElement.FirstIndex = 0;
			BatchElement.NumPrimitives = Indices.Num() / 3;
			BatchElement.MinVertexIndex = 0;
			BatchElement.MaxVertexIndex = Indices.Num() - 1;
			Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
			Mesh.Type = PT_TriangleList;
			Mesh.DepthPriorityGroup = SDPG_World;
			Mesh.bCanApplyViewModeOverrides = false;

			Collector.AddMesh(ViewIndex, Mesh);
		}
	}
}

FPrimitiveViewRelevance FVoxelCloudSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance    = IsShown(View);
	Result.bDynamicRelevance = true;
	//MaterialProxy->GetPrimitiveViewRelevance(Result);
	return Result;
}

uint32 FVoxelCloudSceneProxy::GetMemoryFootprint() const
{
	return sizeof(*this) + Vertices.GetAllocatedSize() + Indices.GetAllocatedSize();
}

SIZE_T FVoxelCloudSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

void FVoxelCloudSceneProxy::InitVertexFactoryData()
{
	//InitOrUpdateResource(&VertexBuffer);

	FLocalVertexFactory::FDataType Data;

	Data.PositionComponent = FVertexStreamComponent(
		&VertexBuffer,
		0,
		sizeof(FVector3f),
		VET_Float3
	);
	Data.PositionComponentSRV = VertexBuffer.ShaderResourceViewRHI;
	Data.TextureCoordinatesSRV = GNullVertexBuffer.VertexBufferSRV;
	Data.TextureCoordinates.Add(FVertexStreamComponent(&GNullVertexBuffer, 0, 0, VET_Float2));
	Data.TangentsSRV = GNullTangentVertexBuffer.VertexBufferSRV;
	Data.TangentBasisComponents[0] = FVertexStreamComponent(&GNullTangentVertexBuffer, 0, 0, VET_PackedNormal);
	Data.TangentBasisComponents[1] = FVertexStreamComponent(&GNullTangentVertexBuffer, sizeof(FPackedNormal), 0, VET_PackedNormal);
	
	VertexFactory->SetData(Data);
	InitOrUpdateResource(VertexFactory);
}

void FVoxelCloudSceneProxy::InitOrUpdateResource(FRenderResource* Resource)
{
	if (!Resource->IsInitialized())
	{
		Resource->InitResource(FRHICommandListImmediate::Get());
	}
	else
	{
		Resource->UpdateRHI(FRHICommandListImmediate::Get());
	}
}
