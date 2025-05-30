// Fill out your copyright notice in the Description page of Project Settings.

#include "VoxelCloudVolume.h"

#include "VoxelCloudComputeShaders.h"
#include "SimplexNoise.h"
#include "MarchingCubesLookups.h"

AVoxelCloudVolume::AVoxelCloudVolume() {
	CloudRenderer = CreateDefaultSubobject<UCloudRendererComponent>("CloudRenderer");
	Volume = CreateDefaultSubobject<UBoxComponent>("Volume");

	Volume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	SetRootComponent(Volume);
	CloudRenderer->SetupAttachment(Volume);

	PrimaryActorTick.bCanEverTick = true;
	
}

void AVoxelCloudVolume::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);
	
	TotalTime += DeltaSeconds * TimeScale;
	if(!CloudRenderer->IsVisible()) return;
	UpdateCloudRendererMesh();
}
void AVoxelCloudVolume::OnConstruction(const FTransform& Transform) {
	Super::OnConstruction(Transform);

	if(NoiseScale == 0.0f)
		NoiseScale = 0.00001f;

	Volume->SetBoxExtent(Bounds);
	
	CloudRenderer->SetRelativeTransform(FTransform::Identity);
	UpdateCloudRendererBounds();
	//UpdateCloudRendererMesh();
}

void AVoxelCloudVolume::ForceUpdateMesh()
{
	UpdateCloudRendererBounds();
	UpdateCloudRendererMesh();
}

void AVoxelCloudVolume::UpdateCloudRendererMesh() {
	if(!RealTime) return;
	const auto LocalBounds = Volume->CalcLocalBounds().GetBox();
	const auto ExtentFromMin = LocalBounds.GetExtent() * 2;
	
	FUintVector3 VoxelCount(ExtentFromMin / VoxelSize);
	
	const FVector3f Offset = FVector3f(VoxelCount) * VoxelSize * .5f;
			
	FVoxelCloudExistenceComputeShader::FParameters ShaderParameters;
	ShaderParameters.VoxelGridSize = VoxelCount;
	ShaderParameters.VoxelSize = VoxelSize;
	ShaderParameters.TotalTime = TotalTime;
	ShaderParameters.CloudinessThreshold = 1 - Cloudiness;
	ShaderParameters.NoiseScale = NoiseScale;
	ShaderParameters.Offset = Offset;
	ShaderParameters.Roundedness = Roundedness;

	CloudRenderer->UpdateMesh(ShaderParameters);
}

void AVoxelCloudVolume::UpdateCloudRendererBounds() const {
	CloudRenderer->SetBounds(Volume->CalcLocalBounds());
}