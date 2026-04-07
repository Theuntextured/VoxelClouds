// Copyright 2026 The untextured Dev. All Rights Reserved.


#include "VoxelCloudSceneProxy.h"

#include "VoxelCloudChunk.h"
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
	CloudLodBias = Component->CloudLodBias.GetValue(Scalability::GetQualityLevels().ViewDistanceQuality);
	LodZeroChunkSize = Component->LodZeroChunkSize;
	CloudLodDistanceScalingPower = Component->CloudLodDistanceScalingPower;
	
	const int32 MaxVoxelPerSide = FMath::RoundToInt32((LocalBox.GetExtent().GetAbsMax() * 2.0) / VoxelSize);
	const int32 MaxLodZeroPerSide = (MaxVoxelPerSide + LodZeroChunkSize - 1) / LodZeroChunkSize;
	MaxLod = FMath::CeilLogTwo(MaxLodZeroPerSide);
}

FVoxelCloudSceneProxy::~FVoxelCloudSceneProxy()
{
	
}

FConvexVolume TransformFrustumToLocal(const FConvexVolume& WorldFrustum, const FMatrix& LocalToWorld)
{
	FConvexVolume LocalFrustum;
	const auto WorldToLocalMatrix = LocalToWorld.InverseFast();
	for (const FPlane& WorldPlane : WorldFrustum.Planes)
		LocalFrustum.Planes.Add(WorldPlane.TransformBy(WorldToLocalMatrix));

	LocalFrustum.Init();
    
	return LocalFrustum;
}

void FVoxelCloudSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	const bool bIsInGame = ViewFamily.EngineShowFlags.Game;
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (!(VisibilityMap & (1 << ViewIndex))) continue;
		const FSceneView* View = Views[ViewIndex];
		
		FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
		if (!bIsInGame)
		{
			DrawWireBox(PDI, 
			            ProxyToWorld, DesiredLocalBox, 
			            IsIndividuallySelected() ? FColor::Orange : FColor::Cyan, 
			            SDPG_World, 
			            2.0f);
		}
		FVoxelCloudChunk RootChunk(
			this, 
			MaxLod, 
			LocalBox, 
			LocalBox, 
			TransformFrustumToLocal(View->ViewFrustum, ProxyToWorld),
			ProxyToWorld.InverseTransformPosition(View->ViewLocation)
			);
		RootChunk.DrawDebug(PDI);
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
