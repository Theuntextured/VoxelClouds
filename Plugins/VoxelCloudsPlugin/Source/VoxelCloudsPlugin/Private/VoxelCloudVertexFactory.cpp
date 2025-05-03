#include "VoxelCloudVertexFactory.h"

#include "MaterialDomain.h"
#include "MeshMaterialShader.h"

//"VoxelCloudsPluginShaders/LocalVertexFactory.ush"
IMPLEMENT_VERTEX_FACTORY_TYPE(FVoxelCloudVertexFactory, "/Engine/Private/LocalVertexFactory.ush", 
EVertexFactoryFlags::UsedWithMaterials |
EVertexFactoryFlags::SupportsStaticLighting |
EVertexFactoryFlags::SupportsDynamicLighting |
EVertexFactoryFlags::SupportsPrecisePrevWorldPos |
EVertexFactoryFlags::SupportsPositionOnly)

IMPLEMENT_TYPE_LAYOUT(FVoxelCloudVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelCloudVertexFactory, SF_Vertex, FVoxelCloudVertexFactoryShaderParameters);

bool FVoxelCloudVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters) {
	return Parameters.MaterialParameters.MaterialDomain == MD_Surface || Parameters.MaterialParameters.bIsDefaultMaterial;
}
void FVoxelCloudVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters,
	FShaderCompilerEnvironment& OutEnvironment) {
	OutEnvironment.SetDefineIfUnset(TEXT("MANUAL_VERTEX_FETCH"), TEXT("0"));
}
void FVoxelCloudVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
{

	// Check if this vertex factory has a valid feature level that is supported by the current platform
	check(HasValidFeatureLevel());


	//The vertex declaration element lists (Nothing but an array of FVertexElement)
	FVertexDeclarationElementList Elements; //Used for the Default vertex stream
	FVertexDeclarationElementList PosOnlyElements; // Used for the PositionOnly vertex stream

	if (Data.PositionComponent.VertexBuffer != NULL)
	{
		//We add the position stream component to both elemnt lists
		Elements.Add(AccessStreamComponent(Data.PositionComponent, 0));
		PosOnlyElements.Add(AccessStreamComponent(Data.PositionComponent, 0, EVertexInputStreamType::PositionOnly));
	}

	//Initialize the Position Only vertex declaration which will be used in the depth pass
	InitDeclaration(PosOnlyElements, EVertexInputStreamType::PositionOnly);

	//We add all the available texcoords to the default element list, that's all what we'll need for unlit shading
	if (Data.TextureCoordinates.Num())
	{
		const int32 BaseTexCoordAttribute = 4;
		for (int32 CoordinateIndex = 0; CoordinateIndex < Data.TextureCoordinates.Num(); CoordinateIndex++)
		{
			Elements.Add(AccessStreamComponent(
				Data.TextureCoordinates[CoordinateIndex],
				BaseTexCoordAttribute + CoordinateIndex
			));
		}

		for (int32 CoordinateIndex = Data.TextureCoordinates.Num(); CoordinateIndex < MAX_STATIC_TEXCOORDS / 2; CoordinateIndex++)
		{
			Elements.Add(AccessStreamComponent(
				Data.TextureCoordinates[Data.TextureCoordinates.Num() - 1],
				BaseTexCoordAttribute + CoordinateIndex
			));
		}
	}

	check(Streams.Num() > 0);

	InitDeclaration(Elements);
	check(IsValidRef(GetDeclaration()));
}