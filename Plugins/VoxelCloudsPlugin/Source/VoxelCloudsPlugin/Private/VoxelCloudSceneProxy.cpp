// Copyright 2026 The untextured Dev. All Rights Reserved.


#include "VoxelCloudSceneProxy.h"
#include "VoxelCloudComponent.h"

FVoxelCloudSceneProxy::FVoxelCloudSceneProxy(UVoxelCloudComponent* Component)
	: FPrimitiveSceneProxy(Component)
{
	check(Component);
	ProxyToWorld = Component->GetComponentTransform().ToMatrixNoScale();
	FVector BoxBounds = Component->Bounds * Component->GetComponentScale();
	DesiredLocalBox = FBox(-BoxBounds, BoxBounds);
	VoxelSize = Component->VoxelSize;
	BoxBounds = FVector(FIntVector(BoxBounds / VoxelSize)) * VoxelSize;
	LocalBox = FBox(-BoxBounds, BoxBounds);
}

FVoxelCloudSceneProxy::~FVoxelCloudSceneProxy()
{
}

#include "SceneManagement.h"

void FVoxelCloudSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
			
			DrawWireBox(PDI, 
				ProxyToWorld, DesiredLocalBox, 
				IsIndividuallySelected() ? FColor::Orange : FColor::Cyan, 
				SDPG_World, 
				2.0f);
			
			if (IsIndividuallySelected())
			{
				for (double x = LocalBox.Min.X; x <= LocalBox.Max.X; x += VoxelSize)
					for (double y = LocalBox.Min.Y; y <= LocalBox.Max.Y; y += VoxelSize)
							for (double z = LocalBox.Min.Z; z <= LocalBox.Max.Z; z += VoxelSize)
							{
								const FVector Position = ProxyToWorld.TransformPosition(FVector(x, y, z));
								PDI->DrawPoint(
									Position,
									FColor::Blue,
									2.f,
									SDPG_World);
							}
			}
		}
	}
}

FPrimitiveViewRelevance FVoxelCloudSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);
    
	Result.bDynamicRelevance = true; 
    
	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
    
	return Result;
}

uint32 FVoxelCloudSceneProxy::GetMemoryFootprint(void) const
{
	return sizeof(*this);
}

SIZE_T FVoxelCloudSceneProxy::GetTypeHash() const
{
	static SIZE_T Unique;
	return reinterpret_cast<SIZE_T>(&Unique);
}
