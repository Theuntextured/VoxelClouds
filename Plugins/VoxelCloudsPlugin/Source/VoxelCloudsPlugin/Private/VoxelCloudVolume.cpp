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

void AVoxelCloudVolume::UpdateCloudRendererMesh() {
	if(IsProcessing) return;
	IsProcessing = true;
	
	const auto LocalBounds = Volume->CalcLocalBounds().GetBox();
	const auto ExtentFromMin = LocalBounds.GetExtent() * 2;
	
	FUintVector3 VoxelCount(ExtentFromMin / VoxelSize);
	
	if(VoxelCount.X <= 0 || VoxelCount.Y <= 0 || VoxelCount.Z <= 0) {
		CloudRenderer->UpdateMesh({},{});
		return;
	}
	
	const FVector3f Offset = FVector3f(VoxelCount) * VoxelSize * .5f;
			
	FVoxelCloudExistenceComputeShader::FParameters ShaderParameters;
	ShaderParameters.VoxelGridSize = VoxelCount;
	ShaderParameters.VoxelSize = VoxelSize;
	ShaderParameters.TotalTime = TotalTime;
	ShaderParameters.CloudinessThreshold = Cloudiness;
	ShaderParameters.NoiseScale = NoiseScale;
	ShaderParameters.Offset = Offset;
	ShaderParameters.Roundedness = Roundedness;

	FVoxelCloudComputeShaders::Dispatch(
		ShaderParameters,
		[this, VoxelCount, ShaderParameters](TArray<VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE> OutputBuffer)
		{
			TArray<uint32> Indices;
			TArray<FVector3f> Vertices;

			Vertices.Reserve(OutputBuffer.Num() * 3);

			for (const VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE& Triangle : OutputBuffer)
			{
				Vertices.Add(Triangle.PosA);
				Vertices.Add(Triangle.PosB);
				Vertices.Add(Triangle.PosC);
			}


			Indices.Reserve(Vertices.Num());
			
			for(uint32 i = 0; i < static_cast<unsigned>(Vertices.Num()); ++i)
				Indices.Add(i);
			
			int32 NonZeroCount = 0;
			for(const auto i : Vertices)
				if(!i.IsZero())
					++NonZeroCount;

			GEngine->AddOnScreenDebugMessage(1244, 1.0f, FColor::Red, "Non-zero percent: " + FString::SanitizeFloat(double(NonZeroCount) / Vertices.Num() * 100) + "%");

			CloudRenderer->UpdateMesh(Vertices, Indices);
			IsProcessing = false;
		}
	);
}

void AVoxelCloudVolume::UpdateCloudRendererBounds() const {
	CloudRenderer->SetBounds(Volume->CalcLocalBounds());
}