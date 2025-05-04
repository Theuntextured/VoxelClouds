#pragma once

class FVoxelCloudVertexFactory : public FLocalVertexFactory
{
public:
	FVoxelCloudVertexFactory(ERHIFeatureLevel::Type InFeatureLevel) : FLocalVertexFactory(InFeatureLevel, "VoxelCloudVertexFactory") {}
};

