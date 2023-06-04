#include "VoxelDensityComputeShader.h"
#include "ComputeShader/Public/VoxelDensityComputeShader/VoxelDensityComputeShader.h"

#include "PixelShaderUtils.h"
#include "RenderCore/Public/RenderGraphUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MaterialShader.h"

DECLARE_STATS_GROUP(TEXT("VoxelDensityComputeShader"), STATGROUP_VoxelDensityComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("VoxelDensityComputeShader Execute"), STAT_VoxelDensityComputeShader_Execute, STATGROUP_VoxelDensityComputeShader);

// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
class COMPUTESHADER_API FVoxelDensityComputeShader : public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FVoxelDensityComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FVoxelDensityComputeShader, FGlobalShader);
	
	
	class FVoxelDensityComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<FVoxelDensityComputeShader_Perm_TEST>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
		// In
		SHADER_PARAMETER(float, Seed)
		SHADER_PARAMETER(int, Resolution)
		// Out
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, Output)

	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER);

		// This shader must support typed UAV load and we are testing if it is supported at runtime using RHIIsTypedUAVLoadSupported
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);

		// FForwardLightingParameters::ModifyCompilationEnvironment(Parameters.Platform, OutEnvironment);
	}
private:
};

// This will tell the engine to create the shader and where the shader entry point is.
//                            ShaderType                            ShaderPath                     Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FVoxelDensityComputeShader, "/ComputeShaderShaders/VoxelDensityComputeShader/VoxelComputeShader.usf", "VoxelDensityComputeShader", SF_Compute);

void FVoxelDensityComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList,
	FVoxelDensityComputeShaderDispatchParams Params, TFunction<void(TArray<float>)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_VoxelDensityComputeShader_Execute);
		DECLARE_GPU_STAT(VoxelDensityComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "VoxelDensityComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, VoxelDensityComputeShader);

		const FVoxelDensityComputeShader::FPermutationDomain PermutationVector;
		
		// Add any static permutation options here
		// PermutationVector.Set<FVoxelDensityComputeShader::FMyPermutationName>(12345);

		TShaderMapRef<FVoxelDensityComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);

		if (ComputeShader.IsValid()) {
			FVoxelDensityComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FVoxelDensityComputeShader::FParameters>();


			const FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateBufferDesc(sizeof(float),
					Params.Resolution * Params.Resolution * Params.Resolution),
				TEXT("OutputBuffer"));

			PassParameters->Seed = Params.Seed;
			PassParameters->Resolution = Params.Resolution;
			PassParameters->Output = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutputBuffer, PF_R32_FLOAT));
			

			auto GroupCount = FComputeShaderUtils::GetGroupCount(
				FIntVector(Params.Resolution, Params.Resolution, Params.Resolution),
				FIntVector(NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER, NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER, NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER));
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteVoxelDensityComputeShader"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
			});

			
			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteVoxelDensityComputeShaderOutput"));
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, OutputBuffer, 0u);

			auto RunnerFunc = [GPUBufferReadback, AsyncCallback, Params](auto&& RunnerFunc) -> void {
				if (GPUBufferReadback->IsReady()) {
					const int32 NumElements = Params.Resolution * Params.Resolution * Params.Resolution; //TODO: actually use resolution
					const int32 BufferSize = sizeof(float) * NumElements;
					const float* Buffer = static_cast<float*>(GPUBufferReadback->Lock(BufferSize));

					TArray<float> OutVal;
					OutVal.Append(Buffer, NumElements);
					
					GPUBufferReadback->Unlock();

					AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutVal]() {
						AsyncCallback(OutVal);
					});

					delete GPUBufferReadback;
				} else {
					AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
						RunnerFunc(RunnerFunc);
					});
				}
			};

			AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
				RunnerFunc(RunnerFunc);
			});
			
		} else {
			#if WITH_EDITOR
				GEngine->AddOnScreenDebugMessage((uint64)42145125184, 6.f, FColor::Red, FString(TEXT("The compute shader has a problem.")));
			#endif

			// We exit here as we don't want to crash the game if the shader is not found or has an error.
			
		}
	}

	GraphBuilder.Execute();
}