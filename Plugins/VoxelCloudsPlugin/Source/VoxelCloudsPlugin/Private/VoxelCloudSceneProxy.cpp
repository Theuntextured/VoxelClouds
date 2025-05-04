#include "VoxelCloudSceneProxy.h"
#include "CloudRenderer.h"
#include "DynamicMeshBuilder.h"

FVoxelCloudSceneProxy::FVoxelCloudSceneProxy(UCloudRendererComponent* Component): FPrimitiveSceneProxy(Component),
	Vertices(Component->Vertices), Indices(Component->Indices)
{
	if (Component->GetMaterial(0) && Component->GetMaterial(0)->GetRenderProxy())
		MaterialProxy = Component->GetMaterial(0)->GetRenderProxy();
	else
		MaterialProxy = nullptr;
}

void FVoxelCloudSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views,
	const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	if(!MaterialProxy) return;
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
	{
		if (VisibilityMap & (1u << ViewIndex))
		{
			FDynamicMeshBuilder MeshBuilder(Views[ViewIndex]->GetFeatureLevel());
			// Add each vertex
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(Gather Cloud Vertices);
				for (int32 i = 0; i < Vertices.Num(); ++i)
					MeshBuilder.AddVertex(FDynamicMeshVertex(Vertices[i]));
			}
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(Gather Cloud Triangles);
				MeshBuilder.AddTriangles(Indices);
			}
			{
                    
				TRACE_CPUPROFILER_EVENT_SCOPE(Get Cloud Mesh);
				// Draw
				MeshBuilder.GetMesh(
					GetLocalToWorld(),
					MaterialProxy,
					SDPG_World,
					false, false, false,
					Collector
				);
			}
		}
	}
}

FPrimitiveViewRelevance FVoxelCloudSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance    = IsShown(View);
	Result.bDynamicRelevance = true;
	//MaterialProxy->GetPrimitiveViewRelevance(Result);
	return Result;
}

uint32 FVoxelCloudSceneProxy::GetMemoryFootprint() const
{
	return sizeof(*this) + Vertices.GetAllocatedSize() + Indices.GetAllocatedSize();
}

SIZE_T FVoxelCloudSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}
