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
		[this, ShaderParameters](TArray<VOXEL_CLOUD_COMPUTE_SHADER_OUTPUT_TYPE> VoxelData)
		{
			TArray<uint32> Indices;
			TArray<FVector3f> Vertices;

			auto PosToIndex = [&](const uint32 X, const uint32 Y, const uint32 Z) -> int32 {
				if(X <= ShaderParameters.VoxelGridSize.X && Y <= ShaderParameters.VoxelGridSize.Y && Z <= ShaderParameters.VoxelGridSize.Z) 
					return X + Y * (ShaderParameters.VoxelGridSize.X + 1) + Z * (ShaderParameters.VoxelGridSize.X + 1) * (ShaderParameters.VoxelGridSize.Y + 1);
				return INDEX_NONE;
			};
			auto GetPointPosition = [&](const uint32 X, const uint32 Y, const uint32 Z) -> FVector3f {
				return FVector3f(X, Y, Z) * ShaderParameters.VoxelSize - ShaderParameters.Offset;
			};
			auto Saturate = [](const float Value)->float {
				if(Value < 0.0f) return 0;
				if(Value > 1.0f) return 1;
				return Value;
			};
			auto GetMappedValueNormalizedClamped = [&](float InputMin, float InputMax, float Value) -> float {
				const float Divisor = InputMax - InputMin;
				if(abs(Divisor) < 0.00001f) {
					return (Value >= InputMax) ? 1 : 0;
				}

				return Saturate((Value - InputMin) / Divisor);
			};
			auto GetValueAtIndex = [&](const int32 Index)-> float
			{
				if(Index == INDEX_NONE)
					return 1.f;
				return VoxelData[Index];
			};

			for(uint32 X = 0; X < ShaderParameters.VoxelGridSize.X; X++)
				for(uint32 Y = 0; Y < ShaderParameters.VoxelGridSize.Y; Y++)
					for(uint32 Z = 0; Z < ShaderParameters.VoxelGridSize.Z; Z++) {
						//Set up the indices of the lookup cell vertices
						FUintVector3 CurrentGridPosition(X, Y, Z);
						int32 OffsetIndices[8];
						for(int32 i = 0; i < 8; i++) {
							const auto NewPos = MarchingCubes::MarchingCubesOffsets[i] + CurrentGridPosition;
							OffsetIndices[i] = PosToIndex(NewPos.X, NewPos.Y, NewPos.Z);
						}
						//Calculate lookup index for triangles
						uint32 LookupIndex = 0;
						for(int32 i = 0; i < 8; i++)
							LookupIndex |= (GetValueAtIndex(OffsetIndices[i]) < ShaderParameters.CloudinessThreshold ? 1u : 0u) << i;


						//Cycle through triangle points and calculate their actual 3d position
						for(int32 Edge, Index = 0; (Edge = MarchingCubes::TriangleTable[LookupIndex][Index]) != INDEX_NONE; Index++) {
							FUintVector2 PointIndices = MarchingCubes::EdgeVertexIndices[Edge];

							FUintVector3 GridPointA = MarchingCubes::MarchingCubesOffsets[PointIndices.X] + CurrentGridPosition;
							FUintVector3 GridPointB = MarchingCubes::MarchingCubesOffsets[PointIndices.Y] + CurrentGridPosition;

							FVector3f WorldPositionA = GetPointPosition(GridPointA.X, GridPointA.Y, GridPointA.Z);
							FVector3f WorldPositionB = GetPointPosition(GridPointB.X, GridPointB.Y, GridPointB.Z);

							float ValueA = GetValueAtIndex(PosToIndex(GridPointA.X, GridPointA.Y, GridPointA.Z));
							float ValueB = GetValueAtIndex(PosToIndex(GridPointB.X, GridPointB.Y, GridPointB.Z));

							float Alpha = GetMappedValueNormalizedClamped(ValueA, ValueB, ShaderParameters.CloudinessThreshold);

							Alpha = FMath::Lerp(0.5f, Alpha, ShaderParameters.Roundedness);

							FVector3f VertexPosition = FMath::Lerp(WorldPositionA, WorldPositionB, Alpha);
							Indices.Add(Vertices.Add(VertexPosition));
						}
					}	
			
			CloudRenderer->UpdateMesh(Vertices, Indices);
			IsProcessing = false;
		}
	);
}

void AVoxelCloudVolume::UpdateCloudRendererBounds() const {
	CloudRenderer->SetBounds(Volume->CalcLocalBounds());
}