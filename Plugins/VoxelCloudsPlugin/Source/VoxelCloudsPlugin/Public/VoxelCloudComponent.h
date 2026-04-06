//Copyright The untextured Dev 2026. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelCloudComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VOXELCLOUDSPLUGIN_API UVoxelCloudComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UVoxelCloudComponent();
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Setter, Category = "VoxelClouds")
	FVector Bounds = FVector(100, 100, 100);
	UPROPERTY(EditAnywhere, Category = "VoxelClouds", meta=(ClampMin="1", ClampMax="1000", UIMin="0", UIMax="1000", SliderExponent="2", Units="cm"))
	double VoxelSize = 100;
	
	void SetBounds(const FVector& InBounds);
private:
	UPROPERTY()
	class UStaticMesh* DebugCubeMesh = nullptr;
	UPROPERTY()
	class UMaterialInterface* DebugMaterial = nullptr;
};
