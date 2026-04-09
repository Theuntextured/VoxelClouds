// Copyright 2026 The untextured Dev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FCloudViewContext
{
	FConvexVolume LocalFrustum;
	FVector LocalLocation;
};

struct FVoxelCloudChunkData
{
	FVoxelCloudChunkData() = default;
	~FVoxelCloudChunkData() { VertexBuffer.ReleaseResource(); IndexBuffer.ReleaseResource(); }
	double ExpiryTime;
	FVertexBuffer VertexBuffer;
	FIndexBuffer IndexBuffer;
	bool bRecentRequest = false;
};

class FVoxelCloudTree
{
	FVoxelCloudTree() = default;
	FVoxelCloudTree(class UVoxelCloudComponent* Component);
	friend class FVoxelCloudChunk;
	
	TArray<class FVoxelCloudChunk> Chunks;
	TMap<uint64, FVoxelCloudChunk*> IdToChunk;
	TMap<uint64, FVoxelCloudChunkData> ChunkDataMap;
	TArray<uint64> LeafNodes;
	TArray<uint64> DirtyNodes;
	double UpdateRate = 0.5;
	
	FMatrix LocalToWorld;
	FBox DesiredLocalBox;
	FBox LocalBox;
	FBox ExtendedBox; // This is the box extended such that the number of LOD0 chunks per side is a power of 2.
	double VoxelSize;
	double CloudLodBias;
	int32 LodZeroChunkSize;
	double CloudLodDistanceScalingPower;
	int32 MaxLod;

	int32 AddNode(uint32 InLodLevel,
	              const FBox& InNodeBounds, const TArray<FCloudViewContext>& Views, uint64 InNodeId = 0);
	void Generate(uint32 InLodLevel,
	              const FBox& InNodeBounds, const TArray<FCloudViewContext>& Views);

public:
	void Tick(double DeltaTime);
	void DrawDebug(class FPrimitiveDrawInterface* PDI, double WidthMult = 20.) const;
	void PrepareRenderingData(FRDGBuilder& GraphBuilder);
};

/**
 * 
 */
class VOXELCLOUDSPLUGIN_API FVoxelCloudChunk
{
public:
	FVoxelCloudChunk() = default;
	FVoxelCloudChunk(FVoxelCloudTree& Tree, uint32 InLodLevel, const FBox& InNodeBounds, const TArray<FCloudViewContext>& Views, uint64 InNodeId = 0);
	
	uint32 LodLevel;
	bool bIsInvisible = false;
	bool bIsValid = false;
	FBox NodeBounds;
	uint64 NodeId;
	TArray<int32> Children;
	
	FORCEINLINE bool IsLeaf() const { return Children.IsEmpty(); }
	FORCEINLINE bool IsInner() const { return !IsLeaf(); }
	FORCEINLINE bool ShouldRender() const { return !bIsInvisible && bIsValid && IsLeaf(); }
	
	void DrawDebug(class FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, double WidthMult = 20.) const;
};
