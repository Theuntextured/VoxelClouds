// Copyright 2026 The untextured Dev. All Rights Reserved.


#include "VoxelCloudMaterialExpressionOutput.h"

#include "MaterialCompiler.h"

UVoxelCloudMaterialExpressionOutput::UVoxelCloudMaterialExpressionOutput()
{
	bShowOutputNameOnPin = true;
	MenuCategories.Add(FText::FromString(TEXT("Voxel Clouds")));
}

int32 UVoxelCloudMaterialExpressionOutput::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
	if (!Density.GetTracedInput().Expression)
		return Compiler->Constant(0.0f);

	const int32 DensityCode = Density.Compile(Compiler);

	return Compiler->CustomOutput(this, OutputIndex, DensityCode);
}

void UVoxelCloudMaterialExpressionOutput::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Voxel Cloud Density"));
}

FName UVoxelCloudMaterialExpressionOutput::GetInputName(int32 InputIndex) const
{
	return TEXT("Density");
}
