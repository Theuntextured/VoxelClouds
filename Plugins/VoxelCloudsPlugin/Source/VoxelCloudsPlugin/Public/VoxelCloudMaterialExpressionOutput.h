// Copyright 2026 The untextured Dev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "Materials/MaterialExpressionLandscapeGrassOutput.h"
#include "VoxelCloudMaterialExpressionOutput.generated.h"

/**
 * 
 */
UCLASS(collapsecategories, hidecategories=Object, DisplayName="Voxel Cloud Density")
class VOXELCLOUDSPLUGIN_API UVoxelCloudMaterialExpressionOutput : public UMaterialExpressionCustomOutput
{
	GENERATED_BODY()
	
public:
	UVoxelCloudMaterialExpressionOutput();

	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Density function for voxel generation. Value is clamped [0,1]."))
	FExpressionInput Density;

#if WITH_EDITOR
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual FName GetInputName(int32 InputIndex) const override;

	virtual EMaterialValueType GetInputValueType(int32 InputIndex) override { return MCT_Float; }
#endif

	virtual FString GetFunctionName() const override { return TEXT("GetVoxelCloudDensity"); }
};
