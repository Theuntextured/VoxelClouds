#pragma once

class VOXELCLOUDSPLUGIN_API FNullTangentBuffer : public FVertexBuffer
{
public:
	virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
	virtual void ReleaseRHI() override;

	FShaderResourceViewRHIRef VertexBufferSRV;
};

extern VOXELCLOUDSPLUGIN_API TGlobalResource<FNullTangentBuffer, FRenderResource::EInitPhase::Pre> GNullTangentVertexBuffer;

