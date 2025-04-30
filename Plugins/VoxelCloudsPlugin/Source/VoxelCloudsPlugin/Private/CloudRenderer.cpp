#include "CloudRenderer.h"
#include "DynamicMeshBuilder.h"
#include "Materials/MaterialRenderProxy.h"

class FVoxelCloudSceneProxy : public FPrimitiveSceneProxy
{
    const TArray<FVector3f> Vertices;
    const TArray<uint32> Indices;
    const FMaterialRenderProxy* MaterialProxy;
    UCloudRendererComponent* OwningComponent;

public:
    FVoxelCloudSceneProxy(UCloudRendererComponent* Component)
        : FPrimitiveSceneProxy(Component)
        , Vertices(Component->Vertices)
        , Indices(Component->Indices)
        , OwningComponent(Component)
    {
        if (Component->GetMaterial(0) && Component->GetMaterial(0)->GetRenderProxy())
            MaterialProxy = Component->GetMaterial(0)->GetRenderProxy();
        else
            MaterialProxy = nullptr;
    }

    virtual void GetDynamicMeshElements(
        const TArray<const FSceneView*>& Views,
        const FSceneViewFamily& ViewFamily,
        uint32 VisibilityMap,
        FMeshElementCollector& Collector
    ) const override
    {
        if(!MaterialProxy) return;
        const UE::Math::TTransform<float> WorldTransform = UE::Math::TTransform<float>(OwningComponent->GetComponentTransform());
        for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
        {
            if (VisibilityMap & (1u << ViewIndex))
            {
                FDynamicMeshBuilder MeshBuilder(Views[ViewIndex]->GetFeatureLevel());
                // Add each vertex
                {
                    TRACE_CPUPROFILER_EVENT_SCOPE(Gather Cloud Vertices);
                    for (int32 i = 0; i < Vertices.Num(); ++i)
                    {
                        MeshBuilder.AddVertex(
                            FDynamicMeshVertex(
                                WorldTransform.TransformPosition(Vertices[i])
                                )
                        );
                    }
                }
                {
                    TRACE_CPUPROFILER_EVENT_SCOPE(Gather Cloud Triangles);
                    MeshBuilder.AddTriangles(Indices);
                }
                {
                    
                    TRACE_CPUPROFILER_EVENT_SCOPE(Get Cloud Mesh);
                    // Draw
                    MeshBuilder.GetMesh(
                        FMatrix::Identity,
                        MaterialProxy,
                        SDPG_World,
                        false, false, false,
                        Collector
                    );
                }
            }
        }
    }

    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
    {
        FPrimitiveViewRelevance Result;
        Result.bDrawRelevance    = IsShown(View);
        Result.bDynamicRelevance = true;
        //MaterialProxy->GetPrimitiveViewRelevance(Result);
        return Result;
    }

    virtual uint32 GetMemoryFootprint() const override
    {
        return sizeof(*this) + Vertices.GetAllocatedSize() + Indices.GetAllocatedSize();
    }

    virtual SIZE_T GetTypeHash() const override {
        static size_t UniquePointer;
        return reinterpret_cast<size_t>(&UniquePointer);
    }
};

FPrimitiveSceneProxy* UCloudRendererComponent::CreateSceneProxy()
{
    return new FVoxelCloudSceneProxy(this);
}


FBoxSphereBounds UCloudRendererComponent::CalcBounds(const FTransform& LocalToWorld) const
{
    if(Bounds.SphereRadius > 0.1f)
        return Bounds.TransformBy(LocalToWorld);

    //Calculate them
	if (Vertices.Num())
	{
		UE::Math::TBox<float> Box(Vertices);
		return FBox(Box).TransformBy(LocalToWorld);
	}
	return FBoxSphereBounds(FSphere(FVector::ZeroVector, 0.f));
}

void UCloudRendererComponent::UpdateMesh(const TArray<FVector3f>& NewVerts, const TArray<uint32>& NewIndices)
{
	Vertices = NewVerts;
	Indices  = NewIndices;

	// Tell the renderer to refresh dynamic data each frame
	MarkRenderDynamicDataDirty();
    MarkRenderStateDirty();
}
void UCloudRendererComponent::SetBounds(const FBoxSphereBounds& NewBounds) {
    Bounds = NewBounds;
    MarkRenderTransformDirty();
}

