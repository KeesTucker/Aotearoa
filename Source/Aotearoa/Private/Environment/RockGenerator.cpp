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
	
	FComputeShaderInterface::Dispatch(Params, [this](const TArray<float>& Voxels) {
		auto [Vertices, Triangles] = FMarchingCubesUtility::GenerateMesh(Resolution, Scale, Isolevel, Voxels);
	
		const auto StaticMesh = FStaticMeshGeneration::GenerateStaticMesh(SavePath, Name, Vertices, Triangles, Mat);
		
		StaticMeshComponent->SetStaticMesh(StaticMesh);
	});
}



