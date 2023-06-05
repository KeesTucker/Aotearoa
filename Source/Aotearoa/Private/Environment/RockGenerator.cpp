// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/RockGenerator.h"

#include "MeshDescription.h"
#include "Environment/MarchingCubesUtility.h"
#include "Environment/StaticMeshGeneration.h"
#include "Environment/VoxelGeneration.h"
#include "Dispatch/VoxelDensityComputeShader.h"
#include "Readback/BufferReadbackManager.h"

// Sets default values
ARockGenerator::ARockGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    SetRootComponent(StaticMeshComponent);
}

void ARockGenerator::Tick(float DeltaTime)
{
	auto ReadbackManagerInstance = FBufferReadbackManager::GetInstance();
	ReadbackManagerInstance.Tick();
}

// Called when changes are made in details window
void ARockGenerator::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	
	if (RegenOnEdit)
	{
		GenerateAndUpdateMesh();
	}
}

void ARockGenerator::GenerateAndUpdateMesh()
{
	int Resolution = Size * ResolutionPerUnit;
	Resolution = (Resolution / NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER) * NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER;
	const float Scale = Size / Resolution;

	TArray<FComputeNoiseLayer> ComputeNoiseLayers;
	for (const FNoiseLayer& NoiseLayer : NoiseLayers)
	{
		FComputeNoiseLayer ComputeNoiseLayer(NoiseLayer.ToComputeNoiseLayer());
		ComputeNoiseLayers.Add(ComputeNoiseLayer);
	}
	
	/*FDateTime StartTime = FDateTime::UtcNow();
	auto Voxels = FVoxelGeneration::GenerateVoxelsWithNoise(Resolution, Seed, ShapeModifier, NoiseLayers);
	
	auto [Vertices, Triangles] = FMarchingCubesUtility::GenerateMesh(Resolution, Scale, Isolevel, Voxels);
	const FDateTime EndTime = FDateTime::UtcNow();
	const float ExecutionTime = (EndTime - StartTime).GetTotalSeconds();
	Debug::LogFloat(TEXT("Time Taken:"), ExecutionTime);

	const auto StaticMesh = FStaticMeshGeneration::GenerateStaticMesh(SavePath, Name, Vertices, Triangles, Mat);

	StaticMeshComponent->SetStaticMesh(StaticMesh);*/

	const FDispatchParams Params(Seed, Resolution, static_cast<int>(ShapeModifier), ComputeNoiseLayers, Scale, Isolevel);

	TArray<uint32> Tris;
	TArray<FVector3f> Verts;

	bool TrisCompleted = false;
	bool VertsCompleted = false;
	
	const FDateTime StartTime = FDateTime::UtcNow();

	auto CompleteCheckCallback = [this, &Tris, &Verts, &TrisCompleted, &VertsCompleted, &StartTime]() {
		if (TrisCompleted && VertsCompleted) {
			TrisCompleted = false;
			VertsCompleted = false;
			MeshGenerateCallback(Tris, Verts, StartTime);
		}
	};
	
	FComputeShaderInterface::Dispatch(Params,
		[this, &Tris, &TrisCompleted, &CompleteCheckCallback](const TArray<uint32>& InTris) {
		Tris = InTris;
		TrisCompleted = true;
		CompleteCheckCallback();
	},
	[this, &Verts, &VertsCompleted, &CompleteCheckCallback](const TArray<FVector3f>& InVerts) {
		Verts = InVerts;
		VertsCompleted = true;
		CompleteCheckCallback();
	});
}

void ARockGenerator::MeshGenerateCallback(const TArray<uint32>& Tris, const TArray<FVector3f>& Verts, const FDateTime StartTime) const
{
	const FDateTime EndTime = FDateTime::UtcNow();
	const float ExecutionTime = (EndTime - StartTime).GetTotalSeconds();
	Debug::LogFloat(TEXT("Time Taken:"), ExecutionTime);
	int Length = 0;
	for (const auto TriIndex : Tris)
	{
		if (TriIndex == -1)
		{
			break;
		}
		Length++;
	}
		
	TArray<uint32> TrisCleaned;
	TrisCleaned.Append(Tris);
	TrisCleaned.SetNum(Length);

	TArray<FVector3f> VertsCleaned;
	VertsCleaned.Append(Verts);
	VertsCleaned.SetNum(Length);

	Debug::LogInt(TEXT("OriginalLength:"), Tris.Num());
	Debug::LogInt(TEXT("Length:"), Length);
		
	const auto StaticMesh = FStaticMeshGeneration::GenerateStaticMesh(SavePath, Name, VertsCleaned, TrisCleaned, Mat);
		
	StaticMeshComponent->SetStaticMesh(StaticMesh);
}



