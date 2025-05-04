#pragma once
#include "VoxelCloudVertexFactory.h"

class FVoxelCloudIndexBuffer : public FIndexBuffer
{
public:
	void SetData(const TArray<uint32>& InIndices);

	virtual void ReleaseResource() override;
	
	FShaderResourceViewRHIRef ShaderResourceViewRHI;
	//FUnorderedAccessViewRHIRef UnorderedAccessViewRHI;
};

class FVoxelCloudVertexBuffer : public FVertexBufferWithSRV
{
public:
	void SetData(const TArray<FVector3f>& InPositions);
	//For these, just use null buffer
	/*
	FVertexBufferWithSRV TangentBuffer;
	FVertexBufferWithSRV ColorBuffer;
	FVertexBufferWithSRV TexCoordBuffer;
	*/
};

class FVoxelCloudSceneProxy : public FPrimitiveSceneProxy
{
    const TArray<FVector3f> Vertices;
    const TArray<uint32> Indices;
    const FMaterialRenderProxy* MaterialProxy;

public:
    FVoxelCloudSceneProxy(class UCloudRendererComponent* Component);
	virtual void CreateRenderThreadResources() override;
	virtual void DestroyRenderThreadResources() override;

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily,
                                        uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

    virtual uint32 GetMemoryFootprint() const override;

    virtual SIZE_T GetTypeHash() const override;

private:
	void InitVertexFactoryData();
	static void InitOrUpdateResource(FRenderResource* Resource);

    FVoxelCloudVertexFactory* VertexFactory = nullptr;
	FVoxelCloudIndexBuffer IndexBuffer;
	FVoxelCloudVertexBuffer VertexBuffer;
};