#include "VoxelDensityComputeShader.h"
#include "ComputeShader/Public/VoxelDensityComputeShader/VoxelDensityComputeShader.h"

#include "PixelShaderUtils.h"
#include "RenderCore/Public/RenderGraphUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"

DECLARE_STATS_GROUP(TEXT("VoxelDensityComputeShader"), STATGROUP_VoxelDensityComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("VoxelDensityComputeShader Execute"), STAT_VoxelDensityComputeShader_Execute, STATGROUP_VoxelDensityComputeShader);

// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
class COMPUTESHADER_API FVoxelDensityComputeShader : public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FVoxelDensityComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FVoxelDensityComputeShader, FGlobalShader);
	
	class FVoxelDensityComputeShader_Perm_Test : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<FVoxelDensityComputeShader_Perm_Test>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
		// In
		SHADER_PARAMETER(float, Seed)
		SHADER_PARAMETER(int, Resolution)
		SHADER_PARAMETER(int, ShapeModifier)
		SHADER_PARAMETER(int, NoiseLayersLength)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FComputeNoiseLayer>, NoiseLayers)
		
		// Out
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, Voxels)

	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		OutEnvironment.SetDefine(TEXT("THREADS"), NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER);
	}
};

// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
class COMPUTESHADER_API FMarchingCubesComputeShader : public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FMarchingCubesComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FMarchingCubesComputeShader, FGlobalShader);
	
	class FMarchingCubesComputeShader_Perm_Test : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<FMarchingCubesComputeShader_Perm_Test>;
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
		// In
		SHADER_PARAMETER(int, Resolution)
		SHADER_PARAMETER(float, Scale)
		SHADER_PARAMETER(float, IsoLevel)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, Voxels)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<int>, Counters)
		
		// Out
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FVector3f>, Verts)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<int>, Tris)

	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		OutEnvironment.SetDefine(TEXT("THREADS"), NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER);
	}
};

// This will tell the engine to create the shader and where the shader entry point is.
//                            ShaderType                            ShaderPath                     Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FVoxelDensityComputeShader, "/ComputeShaderShaders/VoxelDensityComputeShader/VoxelDensityComputeShader.usf", "VoxelDensityComputeShader", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FMarchingCubesComputeShader, "/ComputeShaderShaders/VoxelDensityComputeShader/MarchingCubesComputeShader.usf", "MarchingCubesComputeShader", SF_Compute);

void FComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList,
	FDispatchParams Params, TFunction<void(const TArray<uint32>& Tris, const TArray<FVector3f>& Verts)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_VoxelDensityComputeShader_Execute);
		DECLARE_GPU_STAT(VoxelDensityComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "VoxelDensityComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, VoxelDensityComputeShader);

		const FVoxelDensityComputeShader::FPermutationDomain VoxelDensityPermutationVector;
		const FMarchingCubesComputeShader::FPermutationDomain MarchingCubesPermutationVector;

		auto GroupCount = FComputeShaderUtils::GetGroupCount(
			FIntVector(Params.Resolution, Params.Resolution, Params.Resolution),
			FIntVector(NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER, NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER, NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER));

		FVoxelDensityComputeShader::FParameters* PassParametersVoxels = GraphBuilder.AllocParameters<FVoxelDensityComputeShader::FParameters>();
		FMarchingCubesComputeShader::FParameters* PassParametersMarchingCubes = GraphBuilder.AllocParameters<FMarchingCubesComputeShader::FParameters>();
		
		TShaderMapRef<FVoxelDensityComputeShader> VoxelDensityComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), VoxelDensityPermutationVector);
		TShaderMapRef<FMarchingCubesComputeShader> MarchingCubesComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), MarchingCubesPermutationVector);

		const uint64 NoiseLayersTotalSize = sizeof(FComputeNoiseLayer) * Params.NoiseLayers.Num();
		const FRDGBufferRef NoiseLayerBuffer = CreateStructuredBuffer(
			GraphBuilder, TEXT("NoiseLayerBuffer"), sizeof(FComputeNoiseLayer),
			Params.NoiseLayers.Num(), Params.NoiseLayers.GetData(), NoiseLayersTotalSize
		);
			
		const FRDGBufferRef VoxelBuffer = GraphBuilder.CreateBuffer(
			FRDGBufferDesc::CreateBufferDesc(sizeof(float),
				Params.Resolution * Params.Resolution * Params.Resolution),TEXT("VoxelBuffer"));

		const TArray InitCounters = {0};
		const FRDGBufferRef Counters = CreateStructuredBuffer(
			GraphBuilder, TEXT("Counters"), sizeof(int),
			1, InitCounters.GetData(), sizeof(int));

		TArray<float> InitVerts;
		InitVerts.Init(-1.f, static_cast<int>(Params.Resolution * Params.Resolution * Params.Resolution * 0.5f * 3));
		const FRDGBufferRef VertBuffer = CreateStructuredBuffer(
			GraphBuilder, TEXT("VertBuffer"), sizeof(float),
			static_cast<int>(Params.Resolution * Params.Resolution * Params.Resolution * 0.5f * 3),
			InitVerts.GetData(), static_cast<int>(sizeof(float) * Params.Resolution * Params.Resolution * Params.Resolution * 0.5f * 3));

		TArray<int> InitTris;
		InitTris.Init(-1, static_cast<int>(Params.Resolution * Params.Resolution * Params.Resolution * 0.5f));
		const FRDGBufferRef TriBuffer = CreateStructuredBuffer(
			GraphBuilder, TEXT("TriBuffer"), sizeof(int),
			static_cast<int>(Params.Resolution * Params.Resolution * Params.Resolution * 0.5f),
			InitTris.GetData(), static_cast<int>(sizeof(int) * Params.Resolution * Params.Resolution * Params.Resolution * 0.5f));
		
		if (VoxelDensityComputeShader.IsValid())
		{
			PassParametersVoxels->Seed = Params.Seed;
			PassParametersVoxels->Resolution = Params.Resolution;
			PassParametersVoxels->ShapeModifier = Params.ShapeModifier;
			PassParametersVoxels->NoiseLayersLength = Params.NoiseLayersLength;
			PassParametersVoxels->NoiseLayers = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(NoiseLayerBuffer));
			
			PassParametersVoxels->Voxels = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(VoxelBuffer, PF_R32_FLOAT));
			
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteVoxelDensityComputeShader"),
				PassParametersVoxels,
				ERDGPassFlags::Compute,
				[&PassParametersVoxels, VoxelDensityComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, VoxelDensityComputeShader, *PassParametersVoxels, GroupCount);
			});
		}
		else {
#if WITH_EDITOR
			GEngine->AddOnScreenDebugMessage((uint64)42145125184, 6.f, FColor::Red, FString(TEXT("The compute shader has a problem.")));
#endif
			return;
		}

		if (MarchingCubesComputeShader.IsValid())
		{
			PassParametersMarchingCubes->Resolution = Params.Resolution;
			PassParametersMarchingCubes->Scale = Params.Scale;
			PassParametersMarchingCubes->IsoLevel = Params.IsoLevel;
			PassParametersMarchingCubes->Voxels = PassParametersVoxels->Voxels;
			PassParametersMarchingCubes->Counters = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(Counters, PF_R32_SINT));

			PassParametersMarchingCubes->Verts = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(VertBuffer, PF_R32_FLOAT));
			PassParametersMarchingCubes->Tris = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(TriBuffer, PF_R32_SINT));

			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteMarchingCubesComputeShader"),
				PassParametersMarchingCubes,
				ERDGPassFlags::Compute,
				[&PassParametersMarchingCubes, MarchingCubesComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, MarchingCubesComputeShader, *PassParametersMarchingCubes, GroupCount);
			});
		}
		else {
#if WITH_EDITOR
			GEngine->AddOnScreenDebugMessage((uint64)42145125184, 6.f, FColor::Red, FString(TEXT("The compute shader has a problem.")));
#endif
			return;
		}
		
		FRHIGPUBufferReadback* TriGPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteTriOutput"));
		FRHIGPUBufferReadback* VertGPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteVertOutput"));
		AddEnqueueCopyPass(GraphBuilder, TriGPUBufferReadback, TriBuffer, 0u);
		AddEnqueueCopyPass(GraphBuilder, VertGPUBufferReadback, VertBuffer, 0u);

		auto RunnerFunc = [TriGPUBufferReadback, VertGPUBufferReadback, AsyncCallback, Params](auto&& RunnerFunc) -> void {
			if (TriGPUBufferReadback->IsReady() && VertGPUBufferReadback->IsReady()) {
				const int32 NumElements = static_cast<int>(Params.Resolution * Params.Resolution * Params.Resolution * 0.5f);
				int32 BufferSize = sizeof(uint32) * NumElements;
				const uint32* TriBuffer = static_cast<uint32*>(TriGPUBufferReadback->Lock(BufferSize));

				TArray<uint32> Tris;
				Tris.Append(TriBuffer, NumElements);
				
				TriGPUBufferReadback->Unlock();
				
				BufferSize = sizeof(FVector3f) * NumElements;
				const FVector3f* VertBuffer = static_cast<FVector3f*>(VertGPUBufferReadback->Lock(BufferSize));
				
				TArray<FVector3f> Verts;
				Verts.Append(VertBuffer, NumElements);
				
				VertGPUBufferReadback->Unlock();

				AsyncTask(ENamedThreads::GameThread, [AsyncCallback, Tris, Verts]() {
					AsyncCallback(Tris, Verts);
				});

				delete TriGPUBufferReadback;
				delete VertGPUBufferReadback;
			} else {
				AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
					RunnerFunc(RunnerFunc);
				});
			}
		};

		AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
			RunnerFunc(RunnerFunc);
		});
	}

	GraphBuilder.Execute();
}