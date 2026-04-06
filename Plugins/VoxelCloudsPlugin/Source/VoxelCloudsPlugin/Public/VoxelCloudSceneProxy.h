// Copyright 2026 The untextured Dev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class VOXELCLOUDSPLUGIN_API FVoxelCloudSceneProxy : public FPrimitiveSceneProxy
{
public:
	FVoxelCloudSceneProxy(class UVoxelCloudComponent* Component);
	virtual ~FVoxelCloudSceneProxy() override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	virtual uint32 GetMemoryFootprint(void) const override;
	virtual SIZE_T GetTypeHash() const override;
	
private:
	FMatrix ProxyToWorld;
	FBox DesiredLocalBox;
	FBox LocalBox;
	double VoxelSize;
};
