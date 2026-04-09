// Copyright 2026 The untextured Dev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "VoxelCloudChunk.h"

/**
 * 
 */
class VOXELCLOUDSPLUGIN_API FVoxelCloudSceneViewExtension : FSceneViewExtensionBase
{
	friend class FVoxelCloudSceneProxy;
public:
	FVoxelCloudSceneViewExtension(const FAutoRegister& AutoReg, class UVoxelCloudComponent* Component);
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;
	
};
