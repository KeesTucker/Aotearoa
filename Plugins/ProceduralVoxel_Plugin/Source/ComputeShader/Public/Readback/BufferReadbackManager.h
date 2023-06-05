#pragma once

#include "BufferReadbackManager.h"

#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "RHIGPUReadback.h"
#include "UnifiedBuffer.h"

struct FBaseReadbackInfo
{
    virtual ~FBaseReadbackInfo() {}
    virtual bool ReadBuffer() = 0; 
};

template<typename TReadback, typename TCallback>
struct TFReadbackInfo final : FBaseReadbackInfo
{
    typedef TCallback FCallbackType;
    using ReadbackType = TReadback;

    FRHIGPUBufferReadback* BufferReadback;
    FCallbackType Callback;
    int Length;

    TFReadbackInfo(FRHIGPUBufferReadback* InBufferReadback, TCallback InCallback, const int InLength)
        : BufferReadback(InBufferReadback), Callback(InCallback), Length(InLength) { }
    
    virtual bool ReadBuffer() override
    {
        if (BufferReadback->IsReady())
        {
            int32 BufferSize = sizeof(ReadbackType) * Length;
            const ReadbackType* BufferPointer = static_cast<ReadbackType*>(BufferReadback->Lock(BufferSize));

            TArray<ReadbackType> Output;
            Output.Append(BufferPointer, BufferSize);
				
            BufferReadback->Unlock();

            // Execute the callback on the game thread
            AsyncTask(ENamedThreads::GameThread, [Output, this]() {
                Callback(Output);
            });

            return true;
        }
        return false;
    }
};

class FBufferReadbackManager
{
    TArray<TSharedPtr<FBaseReadbackInfo>> BufferReadbacks;

public:
    static FBufferReadbackManager& GetInstance()
    {
        static FBufferReadbackManager Instance;
        return Instance;
    }

    template<typename TReadback, typename TCallback>
    void AddBuffer(FRHIGPUBufferReadback* BufferReadback, TCallback& AsyncCallback, int Length)
    {
        TSharedPtr<TFReadbackInfo<TReadback, TCallback>> ReadbackInfo = MakeShared<TFReadbackInfo<TReadback, TCallback>>(BufferReadback, AsyncCallback, Length);
        BufferReadbacks.Add(MoveTemp(ReadbackInfo));
    }

    void Tick()
    {
        TArray<int> CompletedBuffers;
        for (int i = 0; i < BufferReadbacks.Num(); ++i)
        {
            ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
                [this, i, &CompletedBuffers](FRHICommandListImmediate& RHICmdList)
                {
                    if (BufferReadbacks[i]->ReadBuffer())
                    {
                        CompletedBuffers.Add(i);
                    }
                }
            );
        }
        for (int i = CompletedBuffers.Num() - 1; i >= 0; --i)
        {
            BufferReadbacks.RemoveAt(CompletedBuffers[i]);
        }
    }
};
