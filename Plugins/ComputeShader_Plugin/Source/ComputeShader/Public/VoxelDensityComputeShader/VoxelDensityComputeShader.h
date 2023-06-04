#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "VoxelDensityComputeShader.generated.h"

struct COMPUTESHADER_API FVoxelDensityComputeShaderDispatchParams
{
	float Seed;
	int Resolution;
	
	FVoxelDensityComputeShaderDispatchParams(const int InResolution, const float InSeed)
	: Resolution(InResolution), Seed(InSeed) {}
};

// This is a public interface that we define so outside code can invoke our compute shader.
class COMPUTESHADER_API FVoxelDensityComputeShaderInterface {
public:
	// Executes this shader on the render thread
	static void DispatchRenderThread(
		FRHICommandListImmediate& RHICmdList,
		FVoxelDensityComputeShaderDispatchParams Params,
		TFunction<void(TArray<float>)> AsyncCallback
	);

	// Executes this shader on the render thread from the game thread via EnqueueRenderThreadCommand
	static void DispatchGameThread(
		FVoxelDensityComputeShaderDispatchParams Params,
		TFunction<void(TArray<float> Voxels)> AsyncCallback
	)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
		[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
		{
			DispatchRenderThread(RHICmdList, Params, AsyncCallback);
		});
	}

	// Dispatches this shader. Can be called from any thread
	static void Dispatch(
		const FVoxelDensityComputeShaderDispatchParams& Params,
		const TFunction<void(TArray<float> Voxels)>& AsyncCallback
	)
	{
		if (IsInRenderingThread()) {
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		}else{
			DispatchGameThread(Params, AsyncCallback);
		}
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVoxelDensityComputeShaderLibrary_AsyncExecutionCompleted, const TArray<float>, Value);

UCLASS() // Change the _API to match your project
class COMPUTESHADER_API UVoxelDensityComputeShaderLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	
	// Execute the actual load
	virtual void Activate() override {
		const FVoxelDensityComputeShaderDispatchParams Params(Resolution, Seed);
		
		FVoxelDensityComputeShaderInterface::Dispatch(Params, [this](const TArray<float>& Voxels) {
				Completed.Broadcast(Voxels);
		});
	}
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UVoxelDensityComputeShaderLibrary_AsyncExecution* ExecuteVoxelDensityComputeShader(UObject* WorldContextObject,
		const int Resolution, const float Seed) {
		UVoxelDensityComputeShaderLibrary_AsyncExecution* Action = NewObject<UVoxelDensityComputeShaderLibrary_AsyncExecution>();
		Action->Resolution = Resolution;
		Action->Seed = Seed;
		Action->RegisterWithGameInstance(WorldContextObject);

		return Action;
	}
	

	UPROPERTY(BlueprintAssignable)
	FOnVoxelDensityComputeShaderLibrary_AsyncExecutionCompleted Completed;


	int Resolution;
	float Seed;
};