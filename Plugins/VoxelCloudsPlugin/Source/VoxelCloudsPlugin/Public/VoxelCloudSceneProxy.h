#pragma once
#include "CloudRenderer.h"
#include "VoxelCloudVertexFactory.h"

class FVoxelCloudSceneProxy : public FPrimitiveSceneProxy
{
	const TArray<FVector3f> Vertices;
	const TArray<uint32> Indices;
	const FMaterialRenderProxy* MaterialProxy;

	FMaterialRelevance MaterialRelevance;

public:
	FVoxelCloudSceneProxy(UCloudRendererComponent* Component);
	
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily,
		uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	virtual void CreateRenderThreadResources() override;
	virtual void DestroyRenderThreadResources() override;
	
	virtual uint32 GetMemoryFootprint() const override;
	virtual SIZE_T GetTypeHash() const override;

	static void InitVertexFactoryData(FVoxelCloudVertexFactory* VertexFactory, FPositionVertexBuffer* VertexBuffer);
	static inline void InitOrUpdateResource(FRenderResource* Resource, FRHICommandListImmediate& RHICmdList);
};
