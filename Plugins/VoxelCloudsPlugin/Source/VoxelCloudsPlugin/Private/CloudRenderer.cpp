#include "CloudRenderer.h"
#include "VoxelCloudSceneProxy.h"

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

void UCloudRendererComponent::UpdateMesh(const FVoxelCloudExistenceComputeShader::FParameters& Parameters) {
    if(IsProcessing) return;
    IsProcessing = true;
	
    if(Parameters.VoxelGridSize.X <= 0 || Parameters.VoxelGridSize.Y <= 0 || Parameters.VoxelGridSize.Z <= 0) {
        UpdateMesh({},{});
        return;
    }
			
    FVoxelCloudComputeShaders::Dispatch(
        Parameters,
        [this, Parameters](TArray<FVector3f> Vertices, TArray<uint32> Indices)
        {
            UpdateMesh(Vertices, Indices);
            IsProcessing = false;
        }
    );
}

void UCloudRendererComponent::SetBounds(const FBoxSphereBounds& NewBounds) {
    Bounds = NewBounds;
    MarkRenderTransformDirty();
}

