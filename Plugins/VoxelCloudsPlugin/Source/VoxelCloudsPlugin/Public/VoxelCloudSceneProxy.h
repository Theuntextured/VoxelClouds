#pragma once

class FVoxelCloudSceneProxy : public FPrimitiveSceneProxy
{
    const TArray<FVector3f> Vertices;
    const TArray<uint32> Indices;
    const FMaterialRenderProxy* MaterialProxy;

public:
    FVoxelCloudSceneProxy(class UCloudRendererComponent* Component);

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily,
                                        uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

    virtual uint32 GetMemoryFootprint() const override;

    virtual SIZE_T GetTypeHash() const override;
};