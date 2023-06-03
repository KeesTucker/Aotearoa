// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/RockGenerator.h"

#include "Environment/MarchingCubesUtility.h"
#include "Environment/StaticMeshGeneration.h"
#include "Environment/VoxelGeneration.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ARockGenerator::ARockGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
}

// Called when the game starts or when spawned
void ARockGenerator::BeginPlay()
{
	Super::BeginPlay();

	GenerateAndUpdateMesh();
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
	
	GenerateAndUpdateMesh();
}

void ARockGenerator::GenerateAndUpdateMesh()
{
	Resolution = Size * ResolutionPerUnit;
	Scale = Size / ResolutionPerUnit;
	
	auto Voxels = FVoxelGeneration::GenerateVoxelsWithNoise(Resolution, Seed, ShapeModifier, NoiseLayers);
	
	auto [Vertices, Triangles] = FMarchingCubesUtility::GenerateMesh(Resolution, Scale, Isolevel, Voxels);

	const auto StaticMesh = FStaticMeshGeneration::GenerateStaticMesh(SavePath, Name, Vertices, Triangles);
	
	StaticMeshComponent->SetStaticMesh(StaticMesh);
}



