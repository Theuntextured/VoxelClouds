// Copyright 2026 The untextured Dev. All Rights Reserved.


#include "VoxelCloudChunk.h"

#include "VoxelCloudSceneProxy.h"

FVoxelCloudChunk::FVoxelCloudChunk(const class FVoxelCloudSceneProxy* InSceneProxy, uint32 InLodLevel,
	const FBox& InBounds, const FBox& InNodeBounds, const FConvexVolume& View, const FVector& ViewLocation)
	: SceneProxy(InSceneProxy)
	, LodLevel(InLodLevel)
	, Bounds(InBounds)
{
	check(SceneProxy);
	if (!Bounds.IsValid)
	{
		bIsInvisible = true;
		return;
	}
	if ((bIsInvisible = !View.IntersectBox(InBounds.GetCenter(), InBounds.GetExtent()))) return;
	if (LodLevel == 0) return;
	
	//Distance calculation
	const double SplitDistance = FMath::Pow(LodLevel - 1, SceneProxy->CloudLodDistanceScalingPower) * SceneProxy->CloudLodBias;
	check(SplitDistance >= 0);
	const double DistanceSq = InNodeBounds.ComputeSquaredDistanceToPoint(ViewLocation);
	if (DistanceSq > SplitDistance * SplitDistance) return;
	
	//Split into 8
	Children.Reserve(8);
	const FVector Center = InNodeBounds.GetCenter();
	FVector Vertices[8];
	InNodeBounds.GetVertices(Vertices);
	for (const FVector& Vertex : Vertices)
	{
		const FBox ChildNodeBounds = FBox(FVector::Min(Center, Vertex), FVector::Max(Center, Vertex));
		Children.Emplace(
			InSceneProxy,
			InLodLevel - 1,
			ChildNodeBounds.Overlap(InBounds),
			ChildNodeBounds,
			View,
			ViewLocation
			);
	}
	
}

void FVoxelCloudChunk::DrawDebug(class FPrimitiveDrawInterface* PDI) const
{
	if (!SceneProxy) return;
	if (IsInner())
	{
		for (const auto& Child : Children) Child.DrawDebug(PDI);
		return;
	}
	static FColor Colors[] = {
		FColor::Green,
		FColor::Blue,
		FColor::Yellow,
		FColor::Red,
		FColor::Cyan,
		FColor::Magenta,
		FColor::Orange,
		FColor::Purple,
		FColor::Turquoise,
		FColor::Silver,
		FColor::Emerald,
		FColor::White,
		FColor::Black
	};
	constexpr int32 NumColors = UE_ARRAY_COUNT(Colors);
	DrawWireBox(
		PDI,
		SceneProxy->ProxyToWorld,
		Bounds,
		Colors[LodLevel % NumColors],
		SDPG_World,
		FMath::Exp2(static_cast<float>(LodLevel)) * 10
		);
	/*
	for (double x = Bounds.Min.X; x <= Bounds.Max.X; x += SceneProxy->VoxelSize)
		for (double y = Bounds.Min.Y; y <= Bounds.Max.Y; y += SceneProxy->VoxelSize)
			for (double z = Bounds.Min.Z; z <= Bounds.Max.Z; z += SceneProxy->VoxelSize)
			{
				const FVector Position = SceneProxy->ProxyToWorld.TransformPosition(FVector(x, y, z));
				PDI->DrawPoint(
					Position,
					FColor::Blue,
					2.f,
					SDPG_World);
			}
	*/
}
