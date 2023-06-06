// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/RockGenerator.h"

#include "MeshDescription.h"
#include "Environment/MarchingCubesUtility.h"
#include "Environment/StaticMeshGeneration.h"
#include "Environment/VoxelGeneration.h"
#include "Dispatch/VoxelDensityComputeShader.h"
#include "Readback/BufferReadbackManager.h"

// Sets default values
ARockGenerator::ARockGenerator() : TrisReady(false), VertsReady(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    SetRootComponent(StaticMeshComponent);
}

void ARockGenerator::BeginPlay()
{
	Super::BeginPlay();
	ReadbackManager = FBufferReadbackManager::GetInstance();
}

void ARockGenerator::Tick(float DeltaTime)
{
	ReadbackManager.Tick();
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

void ARockGenerator::CompleteCheckCallback()
{
	if (TrisReady.load() && VertsReady.load()) {
		TrisReady.store(false);
		VertsReady.store(false);
		MeshGenerateCallback();
	}
}

void ARockGenerator::GenerateAndUpdateMesh()
{
	Tris = MakeShared<TArray<uint32>>();
	Verts = MakeShared<TArray<FVector3f>>();
	StartTime = MakeShared<FDateTime>(FDateTime::UtcNow());
	TrisReady.store(false);
	VertsReady.store(false);
	
	int Resolution = Size * ResolutionPerUnit;
	Resolution = (Resolution / NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER) * NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER;
	const float Scale = Size / Resolution;

	TArray<FComputeNoiseLayer> ComputeNoiseLayers;
	for (const FNoiseLayer& NoiseLayer : NoiseLayers)
	{
		FComputeNoiseLayer ComputeNoiseLayer(NoiseLayer.ToComputeNoiseLayer());
		ComputeNoiseLayers.Add(ComputeNoiseLayer);
	}

	const FDispatchParams Params(Seed, Resolution, static_cast<int>(ShapeModifier), ComputeNoiseLayers, Scale, Isolevel);
	
	FComputeShaderInterface::Dispatch(Params,
	[this](const TArray<uint32>& InTris) {
		*Tris = InTris;
		TrisReady.store(true);
		CompleteCheckCallback();
	},
	[this](const TArray<FVector3f>& InVerts) {
		*Verts = InVerts;
		VertsReady.store(true);
		CompleteCheckCallback();
	},
	ReadbackManager);
}

void ARockGenerator::MeshGenerateCallback() const
{
	const FDateTime EndTime = FDateTime::UtcNow();
	const float ExecutionTime = (EndTime - *StartTime).GetTotalSeconds();
	Debug::LogFloat(TEXT("Time Taken:"), ExecutionTime);
	int Length = 0;
	
	for (const auto TriIndex : *Tris)
	{
		if (TriIndex == -1)
		{
			break;
		}
		Length++;
	}
		
	TArray<uint32> TrisCleaned;
	TrisCleaned.Append(*Tris);
	TrisCleaned.SetNum(Length);

	TArray<FVector3f> VertsCleaned;
	VertsCleaned.Append(*Verts);
	VertsCleaned.SetNum(Length);

	Debug::LogInt(TEXT("OriginalLength:"), Tris->Num());
	Debug::LogInt(TEXT("Length:"), Length);
		
	const auto StaticMesh = FStaticMeshGeneration::GenerateStaticMesh(SavePath, Name, VertsCleaned, TrisCleaned, Mat);
		
	StaticMeshComponent->SetStaticMesh(StaticMesh);
}


