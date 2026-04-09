// Copyright 2026 The untextured Dev. All Rights Reserved.


#include "VoxelCloudSceneProxy.h"

#include "VoxelCloudChunk.h"
#include "VoxelCloudComponent.h"
#include "VoxelCloudSceneViewExtension.h"

FVoxelCloudSceneProxy::FVoxelCloudSceneProxy(UVoxelCloudComponent* Component)
	: FPrimitiveSceneProxy(Component)
	, ViewExtension(Component->SceneViewExtension)
	, BoxWireWidth(Component->BoxWireWidth)
{
}

FVoxelCloudSceneProxy::~FVoxelCloudSceneProxy()
{
	
}


void FVoxelCloudSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	const bool bIsInGame = ViewFamily.EngineShowFlags.Game;
	TArray<FCloudViewContext> Contexts;
	Contexts.SetNum(Views.Num());
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (!(VisibilityMap & (1 << ViewIndex))) continue;
		const FSceneView* View = Views[ViewIndex];
		
		FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
		if (!bIsInGame)
			DrawWireBox(PDI, 
			            ViewExtension->ProxyToWorld, ViewExtension->DesiredLocalBox, 
			            IsIndividuallySelected() ? FColor::Orange : FColor::Cyan, 
			            SDPG_World, 
			            2.0f);

		Contexts[ViewIndex] = {
			TransformFrustumToLocal(View->ViewFrustum, ViewExtension->ProxyToWorld),
		  ViewExtension->ProxyToWorld.InverseTransformPosition(View->ViewLocation)
			};
	}
	
	FVoxelCloudChunk RootChunk(
		this, 
		ViewExtension->MaxLod, 
		ViewExtension->LocalBox, 
		ViewExtension->ExtendedBox, 
		Contexts
		);
	
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (!(VisibilityMap & (1 << ViewIndex))) continue;		
		FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
		RootChunk.DrawDebug(PDI, BoxWireWidth);
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
