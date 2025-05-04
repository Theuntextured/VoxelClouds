#include "NullVertexBuffers.h"

void FNullTangentBuffer::InitRHI(FRHICommandListBase& RHICmdList)
{
	FRHIResourceCreateInfo CreateInfo(TEXT("NullTangetnBuffer"));

	VertexBufferRHI = RHICmdList.CreateBuffer(sizeof(FPackedNormal) * 2, BUF_Static | BUF_ShaderResource | BUF_VertexBuffer | BUF_UnorderedAccess, 0, ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask, CreateInfo);

	FPackedNormal* BufferData = static_cast<FPackedNormal*>(RHICmdList.LockBuffer(VertexBufferRHI, 0, sizeof(FPackedNormal) * 2, RLM_WriteOnly));
	BufferData[0] = FPackedNormal(FVector(1, 0, 0));
	BufferData[1] = FPackedNormal(FVector(0, 0, 1));
	BufferData[1].Vector.W = 127;
	RHICmdList.UnlockBuffer(VertexBufferRHI);
	VertexBufferSRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI, sizeof(FPackedNormal), PF_G16R16_SNORM);
}

void FNullTangentBuffer::ReleaseRHI()
{
	VertexBufferRHI.SafeRelease();
	FVertexBuffer::ReleaseRHI();
}

TGlobalResource<FNullTangentBuffer, FRenderResource::EInitPhase::Pre> GNullTangentVertexBuffer;
