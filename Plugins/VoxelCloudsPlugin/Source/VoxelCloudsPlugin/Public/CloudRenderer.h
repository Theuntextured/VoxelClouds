
#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "CloudRenderer.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VOXELCLOUDSPLUGIN_API UCloudRendererComponent : public UMeshComponent
{
	GENERATED_BODY()

	friend class FVoxelCloudSceneProxy;

public:
	UCloudRendererComponent() = default;

	void UpdateMesh(const TArray<FVector3f>& NewVerts, const TArray<uint32>& NewIndices);

	virtual void SetBounds(const FBoxSphereBounds& NewBounds);
	
protected:
	// UMeshComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual int32 GetNumMaterials() const override { return 1; }
	
	TArray<FVector3f> Vertices;
	TArray<uint32> Indices;
	FBoxSphereBounds Bounds;
};
