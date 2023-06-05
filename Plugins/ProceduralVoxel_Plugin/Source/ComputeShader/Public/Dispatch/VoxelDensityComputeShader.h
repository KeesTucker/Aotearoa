#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Readback/BufferReadbackManager.h"

struct COMPUTESHADER_API FComputeNoiseLayer
{
	int NoiseType = 0;
	float Scale = 100.f;
	float Strength = 0.f;
};

struct COMPUTESHADER_API FDispatchParams
{
	float Seed;
	int Resolution;
	int ShapeModifier;
	int NoiseLayersLength;
	TArray<FComputeNoiseLayer> NoiseLayers;

	float Scale;
	float IsoLevel;
	
	FDispatchParams(const float InSeed, const int InResolution, const int InShapeModifier,
		const TArray<FComputeNoiseLayer>& InNoiseLayers, const float InScale, const float InIsoLevel)
	: Seed(InSeed), Resolution(InResolution), ShapeModifier(InShapeModifier),
	NoiseLayersLength(InNoiseLayers.Num()), NoiseLayers(InNoiseLayers), Scale(InScale), IsoLevel(InIsoLevel) {}
};

// This is a public interface that we define so outside code can invoke our compute shader.
class COMPUTESHADER_API FComputeShaderInterface {
public:
	// Executes this shader on the render thread
	static void DispatchRenderThread(
		FRHICommandListImmediate& RHICmdList,
		FDispatchParams Params,
		TFunction<void(const TArray<uint32>& Tris)> TriAsyncCallback,
		TFunction<void(const TArray<FVector3f>& Verts)> VertAsyncCallback,
		FBufferReadbackManager& ReadbackManagerInstance);

	// Executes this shader on the render thread from the game thread via EnqueueRenderThreadCommand
	static void DispatchGameThread(
		FDispatchParams Params,
		TFunction<void(const TArray<uint32>& Tris)> TriAsyncCallback,
		TFunction<void(const TArray<FVector3f>& Verts)> VertAsyncCallback,
		FBufferReadbackManager& ReadbackManagerInstance)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, TriAsyncCallback, VertAsyncCallback, &ReadbackManagerInstance](FRHICommandListImmediate& RHICmdList)
			{
				DispatchRenderThread(RHICmdList, Params, TriAsyncCallback, VertAsyncCallback, ReadbackManagerInstance);
			});
	}

	// Dispatches this shader. Can be called from any thread
	static void Dispatch(
		const FDispatchParams& Params,
		const TFunction<void(const TArray<uint32>& Tris)>& TriAsyncCallback,
		const TFunction<void(const TArray<FVector3f>& Verts)>& VertAsyncCallback,
		FBufferReadbackManager& ReadbackManagerInstance)
	{
		if (IsInRenderingThread()) {
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, TriAsyncCallback, VertAsyncCallback, ReadbackManagerInstance);
		}
		else {
			DispatchGameThread(Params, TriAsyncCallback, VertAsyncCallback, ReadbackManagerInstance);
		}
	}
};