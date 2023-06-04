// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/RockGenerator.h"

#include "Environment/MarchingCubesUtility.h"
#include "Environment/StaticMeshGeneration.h"
#include "Environment/VoxelGeneration.h"
#include "VoxelDensityComputeShader/VoxelDensityComputeShader.h"

// Sets default values
ARockGenerator::ARockGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    SetRootComponent(StaticMeshComponent);
}

// Called when the game starts or when spawned
void ARockGenerator::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ARockGenerator::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
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
	Resolution = Size * ResolutionPerUnit;
	Resolution = (Resolution / NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER) * NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER;
	Scale = Size / Resolution;

	TArray<FComputeNoiseLayer> ComputeNoiseLayers;
	for (const FNoiseLayer& NoiseLayer : NoiseLayers)
	{
		FComputeNoiseLayer ComputeNoiseLayer(NoiseLayer.ToComputeNoiseLayer());
		ComputeNoiseLayers.Add(ComputeNoiseLayer);
	}
	
	const FDispatchParams Params(Seed, Resolution, static_cast<int>(ShapeModifier), ComputeNoiseLayers, Scale, Isolevel);
	
	FComputeShaderInterface::Dispatch(Params, [this](const TArray<uint32>& Tris, const TArray<FVector3f>& Verts) {
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
		
		const auto StaticMesh = FStaticMeshGeneration::GenerateStaticMesh(SavePath, Name, VertsCleaned, TrisCleaned, Mat);
		
		StaticMeshComponent->SetStaticMesh(StaticMesh);
	});
}



