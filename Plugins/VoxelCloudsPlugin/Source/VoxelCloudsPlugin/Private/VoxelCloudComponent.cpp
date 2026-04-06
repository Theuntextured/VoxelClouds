//Copyright The untextured Dev 2026. All rights reserved.

#include "VoxelCloudComponent.h"

#include "VoxelCloudSceneProxy.h"

UVoxelCloudComponent::UVoxelCloudComponent()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
		DebugCubeMesh = CubeAsset.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (MaterialAsset.Succeeded())
		DebugMaterial = MaterialAsset.Object;
}

FPrimitiveSceneProxy* UVoxelCloudComponent::CreateSceneProxy()
{
	return new FVoxelCloudSceneProxy(this);
}

FBoxSphereBounds UVoxelCloudComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(FBox(-Bounds, Bounds)).TransformBy(LocalToWorld);
}


void UVoxelCloudComponent::SetBounds(const FVector& InBounds)
{
	if (Bounds == InBounds)
		return;
	Bounds = InBounds.GetAbs();
	MarkRenderStateDirty();
}
