// Copyright 2026 The untextured Dev. All Rights Reserved.


#include "VoxelCloudSceneViewExtension.h"

#include "VoxelCloudChunk.h"
#include "VoxelCloudComponent.h"

FVoxelCloudSceneViewExtension::FVoxelCloudSceneViewExtension(const FAutoRegister& AutoReg,
                                                             class UVoxelCloudComponent* Component): FSceneViewExtensionBase(AutoReg)
{
	check(Component);
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

void FVoxelCloudSceneViewExtension::PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder,
	FSceneViewFamily& InViewFamily)
{
	const bool bIsInGame = InViewFamily.EngineShowFlags.Game;
	const auto& Views = InViewFamily.Views;
	
	TArray<FCloudViewContext> Contexts;
	Contexts.SetNum(Views.Num());
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		const FSceneView* View = Views[ViewIndex];
		
		Contexts[ViewIndex] = {
			TransformFrustumToLocal(View->ViewFrustum, ProxyToWorld),
		  ProxyToWorld.InverseTransformPosition(View->ViewLocation)
			};
	}
	
	if (RootChunk) delete RootChunk;
	RootChunk = new FVoxelCloudChunk(
		*this, 
		MaxLod, 
		LocalBox, 
		ExtendedBox, 
		Contexts
		);
	
	
}
