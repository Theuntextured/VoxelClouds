#include "VoxelCloudSceneProxy.h"

#include "DynamicMeshBuilder.h"
#include "MaterialDomain.h"
#include "MeshBuilder.h"
#include "Materials/MaterialRenderProxy.h"
#include "PrimitiveUniformShaderParametersBuilder.h"

FVoxelCloudSceneProxy::FVoxelCloudSceneProxy(UCloudRendererComponent* Component)
	: FPrimitiveSceneProxy(Component)
	, Vertices(Component->Vertices)
	, Indices(Component->Indices)
	, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
{
	if (Component->GetMaterial(0) && Component->GetMaterial(0)->GetRenderProxy())
		MaterialProxy = Component->GetMaterial(0)->GetRenderProxy();
	else
		MaterialProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(); 
}

class FVoxelCloudPrimitiveUniformBuffer : public FDynamicPrimitiveResource, public TUniformBuffer<FPrimitiveUniformShaderParameters>
{
public:
	
	// FDynamicPrimitiveResource interface.
	virtual void InitPrimitiveResource(FRHICommandListBase& RHICmdList) override
	{
		InitResource(RHICmdList);
	}
	virtual void ReleasePrimitiveResource() override
	{
		ReleaseResource();
		delete this;
	}
};

class FVoxelCloudOneFrameResources : public FOneFrameResource
{
public:
	FVertexBuffer VertexBuffer;
	FVertexBuffer TangentBuffer;
	FVertexBuffer TexCoordBuffer;
	FVertexBuffer ColorBuffer;
	
	FIndexBuffer IndexBuffer;
	FVoxelCloudVertexFactory* VertexFactory = nullptr;
	FVoxelCloudPrimitiveUniformBuffer* PrimitiveUniformBuffer = nullptr;

	void InitResource(FRHICommandListBase& RHICmdList, const TArray<FVector3f>& Positions, const TArray<uint32>& Indices) {
		FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelCloudOneFrameResourceBuffer"));

		VertexBuffer.VertexBufferRHI = RHICmdList.CreateVertexBuffer(
			Positions.Num() * sizeof(FVector3f),
			BUF_ShaderResource | BUF_VertexBuffer | BUF_Volatile,
			CreateInfo
			);
		VertexBuffer.InitResource(RHICmdList);
		void* Data = RHICmdList.LockBuffer(VertexBuffer.VertexBufferRHI, 0, Positions.Num() * sizeof(FVector3f), RLM_WriteOnly);
		FMemory::Memcpy(Data, Positions.GetData(), Positions.Num() * sizeof(FVector3f));
		RHICmdList.UnlockBuffer(VertexBuffer.VertexBufferRHI);

		IndexBuffer.IndexBufferRHI = RHICmdList.CreateIndexBuffer(
			sizeof(uint32),
			Indices.Num() * sizeof(uint32),
			BUF_IndexBuffer | BUF_ShaderResource | BUF_Volatile,
			CreateInfo);
		IndexBuffer.InitResource(RHICmdList);
		Data = RHICmdList.LockBuffer(IndexBuffer.IndexBufferRHI, 0, Indices.Num() * sizeof(uint32), RLM_WriteOnly);
		FMemory::Memcpy(Data, Indices.GetData(), Indices.Num() * sizeof(uint32));
		RHICmdList.UnlockBuffer(IndexBuffer.IndexBufferRHI);

		TangentBuffer.VertexBufferRHI = RHICmdList.CreateIndexBuffer(
			2 * sizeof(FPackedNormal),
			Positions.Num() * sizeof(FPackedNormal),
			BUF_ShaderResource | BUF_Volatile,
			CreateInfo);
		TangentBuffer.InitResource(RHICmdList);
		FPackedNormal TanX = FVector3f(1, 0, 0);
		FPackedNormal TanZ = FVector3f(0, 0, 1);
		
		FPackedNormal* TangentData = static_cast<FPackedNormal*>(RHICmdList.LockBuffer(TangentBuffer.VertexBufferRHI, 0, Positions.Num() * sizeof(FPackedNormal) * 2, RLM_WriteOnly));
		for(int32 i = 0; i < Positions.Num(); ++i) {
			TangentData[i * 2] = TanX;
			TangentData[i * 2 + 1] = TanZ;
		}
		RHICmdList.UnlockBuffer(TangentBuffer.VertexBufferRHI);

		ColorBuffer.VertexBufferRHI = RHICmdList.CreateIndexBuffer(
			sizeof(uint8) * 4,
			Positions.Num() * sizeof(uint8),
			BUF_ShaderResource | BUF_Volatile,
			CreateInfo);
		ColorBuffer.InitResource(RHICmdList);
		Data = RHICmdList.LockBuffer(ColorBuffer.VertexBufferRHI, 0, Positions.Num() * sizeof(uint8) * 4, RLM_WriteOnly);
		FMemory::Memset(Data, 0, Positions.Num() * sizeof(uint8) * 4);
		RHICmdList.UnlockBuffer(ColorBuffer.VertexBufferRHI);
		
		TexCoordBuffer.VertexBufferRHI = RHICmdList.CreateIndexBuffer(
			sizeof(FVector2f),
			Positions.Num() * sizeof(FVector2f),
			BUF_ShaderResource | BUF_Volatile,
			CreateInfo);
		TexCoordBuffer.InitResource(RHICmdList);
		Data = RHICmdList.LockBuffer(TexCoordBuffer.VertexBufferRHI, 0, Positions.Num() * sizeof(FVector2f), RLM_WriteOnly);
		FMemory::Memset(Data, 0, Positions.Num() * sizeof(FVector2f));
		RHICmdList.UnlockBuffer(TexCoordBuffer.VertexBufferRHI);
		

	}

	virtual VOXELCLOUDSPLUGIN_API ~FVoxelCloudOneFrameResources() override {
		if(VertexBuffer.VertexBufferRHI) {
			VertexBuffer.ReleaseResource();
		}
		if(IndexBuffer.IndexBufferRHI) {
			IndexBuffer.ReleaseResource();
		}
		if(VertexFactory) {
			VertexFactory->ReleaseResource();
			delete VertexFactory;
		}
		if (PrimitiveUniformBuffer)
		{
			PrimitiveUniformBuffer->ReleaseResource();
			delete PrimitiveUniformBuffer;
		}
		if(TangentBuffer.VertexBufferRHI) {
			TangentBuffer.ReleaseResource();
		}
		if(TexCoordBuffer.VertexBufferRHI) {
			TexCoordBuffer.ReleaseResource();
		}
		if(ColorBuffer.VertexBufferRHI) {
			ColorBuffer.ReleaseResource();
		}
	}
	bool IsValidForRendering() const
	{
		return
		     VertexBuffer.VertexBufferRHI
		&& IndexBuffer.IndexBufferRHI
		  && PrimitiveUniformBuffer
		  && VertexFactory
		&& TangentBuffer.VertexBufferRHI
		&& ColorBuffer.VertexBufferRHI
		&& TexCoordBuffer.VertexBufferRHI;
	}
};

void FVoxelCloudSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views,
                                                   const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const {

	if(Vertices.IsEmpty() || Indices.IsEmpty()) return;

	FRHICommandListBase& RHICmdList = GetImmediateCommandList_ForRenderCommand();
	
	const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;
	auto MaterialRenderProxyToSubmit = MaterialProxy;
	if (bWireframe)
	{
		MaterialRenderProxyToSubmit = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr,
			FLinearColor(0, 0.5f, 1.f)
		);

		Collector.RegisterOneFrameMaterialProxy(const_cast<FMaterialRenderProxy*>(MaterialRenderProxyToSubmit));
	}

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
	{
		if (VisibilityMap & (1u << ViewIndex))
		{
#if 0
			// OLD CODE!!!!
			FDynamicMeshBuilder MeshBuilder(GetScene().GetFeatureLevel(),
				1, 0, true);

			for(const auto& Vertex : Vertices) {
				MeshBuilder.AddVertex(Vertex);
			}
			MeshBuilder.AddTriangles(Indices);
			MeshBuilder.GetMesh(GetLocalToWorld(), MaterialRenderProxyToSubmit, SDPG_World, true, false, ViewIndex, Collector);
			continue;
#endif
		
			FVoxelCloudOneFrameResources& OneFrameResources = Collector.AllocateOneFrameResource<FVoxelCloudOneFrameResources>();

			OneFrameResources.InitResource(RHICmdList, Vertices, Indices);

			{
				OneFrameResources.VertexFactory = new FVoxelCloudVertexFactory(GetScene().GetFeatureLevel());
				FLocalVertexFactory::FDataType Data;

				Data.PositionComponent = FVertexStreamComponent(
					&OneFrameResources.VertexBuffer,
					0,
					sizeof(FVector3f),
					VET_Float3);
				Data.TangentBasisComponents = {
					FVertexStreamComponent(
						&OneFrameResources.TangentBuffer,
						sizeof()
						)
				}

				Data.NumTexCoords = 1;
				Data.LightMapCoordinateIndex = 0;


				
				
				
				Data.TangentsSRV = RHICmdList.CreateShaderResourceView(OneFrameResources.TangentBuffer->VertexBufferRHI,
					sizeof(FPackedNormal), EPixelFormat::PF_R8G8B8A8_SNORM);
				Data.TextureCoordinatesSRV = RHICmdList.CreateShaderResourceView(OneFrameResources.TexCoordBuffer->VertexBufferRHI,
					sizeof(FVector2f), EPixelFormat::PF_G32R32F);
				Data.ColorComponentsSRV = RHICmdList.CreateShaderResourceView(OneFrameResources.ColorBuffer->VertexBufferRHI,
					4, PF_R8G8B8A8);
				
				Data.LODLightmapDataIndex = 0;
				
				OneFrameResources.VertexFactory->SetData(Data);
				OneFrameResources.VertexFactory->InitResource(RHICmdList);
			}
			{
				// Create the primitive uniform buffer.
				OneFrameResources.PrimitiveUniformBuffer = new FVoxelCloudPrimitiveUniformBuffer();
				FPrimitiveUniformShaderParameters PrimitiveParams = FPrimitiveUniformShaderParametersBuilder{}
				.Defaults()
					.LocalToWorld(GetLocalToWorld())
					.PreviousLocalToWorld(GetLocalToWorld())
					.ActorWorldPosition(GetLocalToWorld().GetOrigin())
					.WorldBounds(FBoxSphereBounds(EForceInit::ForceInit))
					.LocalBounds(FBoxSphereBounds(EForceInit::ForceInit))
					.ReceivesDecals(false)
					.OutputVelocity(true)
				.Build();

				OneFrameResources.PrimitiveUniformBuffer->SetContents(PrimitiveParams);
				OneFrameResources.PrimitiveUniformBuffer->InitResource(RHICmdList);
			}

			{
				FMeshBatch& Mesh = Collector.AllocateMesh();
				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer = OneFrameResources.IndexBuffer;
				Mesh.VertexFactory = OneFrameResources.VertexFactory;
				Mesh.MaterialRenderProxy = MaterialRenderProxyToSubmit;
				BatchElement.PrimitiveUniformBufferResource = OneFrameResources.PrimitiveUniformBuffer;
				BatchElement.PrimitiveIdMode = PrimID_DynamicPrimitiveShaderData;
				
				
				Mesh.CastShadow = true;
				Mesh.bWireframe = bWireframe;
				Mesh.bCanApplyViewModeOverrides = true;
				Mesh.bUseWireframeSelectionColoring = true;
				Mesh.ReverseCulling = GetLocalToWorld().Determinant() < 0.f;
				Mesh.bDisableBackfaceCulling = true;
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = SDPG_World;
				Mesh.bUseSelectionOutline = true;

				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = Indices.Num() / 3;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = Indices.Num() - 1;

				Collector.AddMesh(ViewIndex, Mesh);
			}
		}
	}
}
FPrimitiveViewRelevance FVoxelCloudSceneProxy::GetViewRelevance(const FSceneView* View) const {
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bDynamicRelevance = true;
	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
	MaterialRelevance.SetPrimitiveViewRelevance(Result);
	Result.bVelocityRelevance = IsMovable() && Result.bOpaque && Result.bRenderInMainPass;
	return Result;
}

void FVoxelCloudSceneProxy::CreateRenderThreadResources() {

	if(Vertices.IsEmpty() || Indices.IsEmpty()) return;
	
}
void FVoxelCloudSceneProxy::DestroyRenderThreadResources() {
	
}
uint32 FVoxelCloudSceneProxy::GetMemoryFootprint() const {
	return sizeof(*this) + Vertices.GetAllocatedSize() + Indices.GetAllocatedSize();
}
SIZE_T FVoxelCloudSceneProxy::GetTypeHash() const {
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

void FVoxelCloudSceneProxy::InitVertexFactoryData(FVoxelCloudVertexFactory* VertexFactory,
	FPositionVertexBuffer* VertexBuffer) {
	auto Run = [VertexFactory, VertexBuffer](FRHICommandListImmediate& RHICmdList)
	{
		//Initialize or update the RHI vertex buffers
		InitOrUpdateResource(VertexBuffer, RHICmdList);

		//Use the RHI vertex buffers to create the needed Vertex stream components in an FDataType instance, and then set it as the data of the vertex factory
		FLocalVertexFactory::FDataType Data;
		VertexBuffer->BindPositionVertexBuffer(VertexFactory, Data);
		VertexFactory->SetData(Data);

		//Initalize the vertex factory using the data that we just set, this will call the InitRHI() method that we implemented in out vertex factory
		InitOrUpdateResource(VertexFactory, RHICmdList);
	};
	if(IsInActualRenderingThread()) {
		Run(GetImmediateCommandList_ForRenderCommand());
	}
	else {
		ENQUEUE_RENDER_COMMAND(StaticMeshVertexBuffersLegacyInit)(MoveTemp(Run));			
	}
}

void FVoxelCloudSceneProxy::InitOrUpdateResource(FRenderResource* Resource, FRHICommandListImmediate& RHICmdList) {
	if (!Resource->IsInitialized())
	{
		Resource->InitResource(RHICmdList);
	}
	else
	{
		Resource->UpdateRHI(RHICmdList);
	}
}
