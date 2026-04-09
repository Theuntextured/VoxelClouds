// Copyright 2026 The untextured Dev. All Rights Reserved.


#include "VoxelCloudChunk.h"

#include "VoxelCloudComponent.h"


FVoxelCloudTree::FVoxelCloudTree(class UVoxelCloudComponent* Component)
{
	LocalToWorld = Component->GetComponentTransform().ToMatrixNoScale();
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
	
	const FVector Center = LocalBox.GetCenter();
	const FVector ExtendedExtent = FVector(MaxLodZeroPerSide * LodZeroChunkSize * VoxelSize);
	ExtendedBox = FBox(Center - ExtendedExtent, Center + ExtendedExtent);
}

int32 FVoxelCloudTree::AddNode(uint32 InLodLevel,
                               const FBox& InNodeBounds, const TArray<FCloudViewContext>& Views, uint64 InNodeId)
{
	if (Chunks.Emplace_GetRef(*this, InLodLevel, InNodeBounds, Views, InNodeId).IsLeaf())
		LeafNodes.Add(Chunks.Num() - 1);
	IdToChunk.Add(InNodeId, &Chunks.Last());
	return Chunks.Num() - 1;
}

void FVoxelCloudTree::Generate(uint32 InLodLevel, const FBox& InNodeBounds, const TArray<FCloudViewContext>& Views)
{
	IdToChunk.Reset();
	Chunks.Reset();
	LeafNodes.Reset();
	Chunks.Emplace(*this, InLodLevel, InNodeBounds, Views, 0);
	for (const uint64 LeafNode : LeafNodes)
	{
		FVoxelCloudChunkData* Data = ChunkDataMap.Find(LeafNode);
		if (!Data)
		{
			ChunkDataMap.Add(LeafNode).ExpiryTime = UpdateRate;
			DirtyNodes.Add(LeafNode);
		}
		else Data->bRecentRequest = true;
	}
}

void FVoxelCloudTree::Tick(double DeltaTime)
{
	TArray<uint64> ToRemove;
	for (auto& [Id, Data] : ChunkDataMap)
		if (Data.ExpiryTime -= DeltaTime < 0)
		{
			if (Data.bRecentRequest)
			{
				Data.ExpiryTime = UpdateRate;
				Data.bRecentRequest = false;
				DirtyNodes.Add(Id);
				continue;
			}
			ToRemove.Add(Id);
		}
	for (const uint64 Id : ToRemove)
		ChunkDataMap.Remove(Id);
}

void FVoxelCloudTree::DrawDebug(class FPrimitiveDrawInterface* PDI, double WidthMult) const
{
	for (const auto& Chunk : Chunks)
		Chunk.DrawDebug(PDI, LocalToWorld, WidthMult);
}

void FVoxelCloudTree::PrepareRenderingData(FRDGBuilder& GraphBuilder)
{
	if (DirtyNodes.IsEmpty()) return;
	
	for (const uint64 NodeId : DirtyNodes)
	{
		auto& Data = ChunkDataMap[NodeId];
		auto& Chunk = *IdToChunk[NodeId];
		
		
		//Calc num of points
		FBox ActualBounds = Chunk.NodeBounds.Overlap(LocalBox);
		FVector Size = ActualBounds.GetSize();
	}
}

FVoxelCloudChunk::FVoxelCloudChunk(FVoxelCloudTree& Tree, uint32 InLodLevel,
                                   const FBox& InNodeBounds, const TArray<FCloudViewContext>& Views, uint64 InNodeId)
    : LodLevel(InLodLevel)
	, bIsValid(true)
	, NodeBounds((InNodeBounds))
	, NodeId(InNodeId)
{
	check(InLodLevel < 16)
    bIsInvisible = true;
	
    if (!NodeBounds.IsValid) return;

    bool bNeedsSubdivision = false;

    const double SplitDistance = FMath::Pow(LodLevel - 1, Tree.CloudLodDistanceScalingPower) * Tree.CloudLodBias;
    check(SplitDistance >= 0);
    const double SplitDistanceSq = SplitDistance * SplitDistance;

    for (const FCloudViewContext& ViewCtx : Views)
    {
        if (ViewCtx.LocalFrustum.IntersectBox(NodeBounds.GetCenter(), NodeBounds.GetExtent()))
        {
            bIsInvisible = false;
        	
            // Does this camera demand higher detail?
            if (LodLevel > 0 && InNodeBounds.ComputeSquaredDistanceToPoint(ViewCtx.LocalLocation) <= SplitDistanceSq)
            {
                bNeedsSubdivision = true;
                break;
            }
        }
    }

    if (bIsInvisible) return;
    if (!bNeedsSubdivision) return;

    // Split into 8
    const FVector Center = InNodeBounds.GetCenter();
    FVector Vertices[8];
    InNodeBounds.GetVertices(Vertices);
    Children.SetNum(8);
    for (uint64 i = 0; i < 8; ++i)
    {
	    const FVector& Vertex = Vertices[i];
	    const FBox ChildNodeBounds = FBox(FVector::Min(Center, Vertex), FVector::Max(Center, Vertex));
    	const uint64 ChildNodeId = NodeId | (i << (4 * LodLevel));
	    Children[i] = Tree.AddNode(
		    InLodLevel - 1,
		    ChildNodeBounds,
		    Views,
		    ChildNodeId
	    );
    }
}

void FVoxelCloudChunk::DrawDebug(class FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, double WidthMult) const
{
	if (IsInner()) return;
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
		LocalToWorld,
		NodeBounds,
		Colors[LodLevel % NumColors],
		SDPG_World,
		FMath::Exp2(static_cast<float>(LodLevel)) * WidthMult
		);
}
