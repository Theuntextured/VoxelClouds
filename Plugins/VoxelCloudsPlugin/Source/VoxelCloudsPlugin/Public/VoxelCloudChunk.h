// Copyright 2026 The untextured Dev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class VOXELCLOUDSPLUGIN_API FVoxelCloudChunk
{
public:
	FVoxelCloudChunk(const class FVoxelCloudSceneProxy* InSceneProxy, uint32 InLodLevel, const FBox& InBounds,
	                 const FBox& InNodeBounds, const FConvexVolume& View, const FVector& ViewLocation);

	const class FVoxelCloudSceneProxy* SceneProxy;
	
	uint32 LodLevel;
	FBox Bounds;
	
	FORCEINLINE bool IsLeaf() const { return Children.IsEmpty(); }
	FORCEINLINE bool IsInner() const { return !IsLeaf(); }
	
	TArray<FVoxelCloudChunk> Children;
	bool bIsInvisible = false;
	
	void DrawDebug(class FPrimitiveDrawInterface* PDI) const;
};
