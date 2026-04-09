// Copyright 2026 The untextured Dev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class VOXELCLOUDSPLUGIN_API FVoxelCloudSceneProxy : public FPrimitiveSceneProxy
{
	using Super = FPrimitiveSceneProxy;
	friend class FVoxelCloudChunk;
public:
	FVoxelCloudSceneProxy(class UVoxelCloudComponent* Component);
	virtual ~FVoxelCloudSceneProxy() override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	virtual uint32 GetMemoryFootprint(void) const override;
	virtual SIZE_T GetTypeHash() const override;
	
private:
	TSharedRef<class FVoxelCloudSceneViewExtension> ViewExtension;
	double BoxWireWidth = 10;
};
