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
    virtual void ReadBuffer() = 0;
    bool IsCompleted = false;
};

template<typename TReadback, typename TCallback>
struct TFReadbackInfo final : FBaseReadbackInfo
{
    typedef TCallback FCallbackType;

    FRHIGPUBufferReadback* BufferReadback;
    FCallbackType Callback;
    int Length;

    TFReadbackInfo(FRHIGPUBufferReadback* InBufferReadback, TCallback InCallback, const int InLength)
        : BufferReadback(InBufferReadback), Callback(InCallback), Length(InLength) { }
    
    virtual void ReadBuffer() override
    {
        ENQUEUE_RENDER_COMMAND(FReadBufferCommand)(
        [this](FRHICommandListImmediate& RHICmdList)
        {
            if (IsCompleted)
            {
                return;
            }
            if (BufferReadback->IsReady())
            {
                const int32 BufferSize = sizeof(TReadback) * Length;
                const TReadback* BufferPointer = static_cast<TReadback*>(BufferReadback->Lock(BufferSize));
                TArray<TReadback>* Output = new TArray<TReadback>();
                Output->Append(BufferPointer, Length);
                BufferReadback->Unlock();
                IsCompleted = true;
                
                // Execute the callback on the game thread
                AsyncTask(ENamedThreads::GameThread, [Output = TSharedPtr<TArray<TReadback>>(Output), this]() {
                    Callback(*Output);
                });
            }
        });
    }
};

class FBufferReadbackManager
{
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

private:
    FDelegateHandle TickHandle;
    TArray<TSharedPtr<FBaseReadbackInfo>> BufferReadbacks;
    
    FBufferReadbackManager()
    {
        TickHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FBufferReadbackManager::Tick), 1.f);
    }

    ~FBufferReadbackManager()
    {
        FTicker::GetCoreTicker().RemoveTicker(TickHandle);
    }
    
    bool Tick(float DeltaTime)
    {
        TArray<int> CompletedIndices;
        
        for (int i = 0; i < BufferReadbacks.Num(); ++i)
        {
            if (BufferReadbacks[i]->IsCompleted)
            {
                CompletedIndices.Add(i);
                continue;
            }
            BufferReadbacks[i]->ReadBuffer();
        }
        
        // Remove the completed buffer readbacks in reverse order
        for (int i = CompletedIndices.Num() - 1; i >= 0; --i)
        {
            const int CompletedIndex = CompletedIndices[i];
            BufferReadbacks.RemoveAt(CompletedIndex);
        }

        return true;
    }
};