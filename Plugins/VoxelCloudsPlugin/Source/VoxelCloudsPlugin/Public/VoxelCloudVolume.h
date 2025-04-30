// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CloudRenderer.h"
#include "Components/BoxComponent.h"
#include "VoxelCloudVolume.generated.h"

UCLASS(Blueprintable, BlueprintType)
class VOXELCLOUDSPLUGIN_API AVoxelCloudVolume : public AActor
{
	GENERATED_BODY()
public:
	AVoxelCloudVolume();

	virtual void Tick(float DeltaSeconds) override;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCloudRendererComponent* CloudRenderer;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* Volume;

	UPROPERTY(EditAnywhere, Category = "Voxel Clouds")
	float VoxelSize = 100.f;
	UPROPERTY(EditAnywhere, Category = "Voxel Clouds")
	float NoiseScale = 1.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Voxel Clouds")
	double TotalTime = 0.;
	UPROPERTY(EditAnywhere, Category = "Voxel Clouds")
	double TimeScale = 1.0;
	UPROPERTY(EditAnywhere, Category = "Voxel Clouds")
	FVector Bounds = {100, 100, 100};
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Voxel Clouds", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float Cloudiness = 0.5f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Voxel Clouds", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float Roundedness = 0.5f;
	
private:
	void UpdateCloudRendererMesh();
	void UpdateCloudRendererBounds() const;

	bool IsProcessing = false;
};
