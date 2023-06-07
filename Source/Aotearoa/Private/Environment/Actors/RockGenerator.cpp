// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/Actors/RockGenerator.h"

#include "MeshDescription.h"
#include "Environment/MarchingCubes/StaticMeshGeneration.h"
#include "Environment/MarchingCubes/VoxelGeneration.h"
#include "Dispatch/VoxelDensityComputeShader.h"

// Sets default values
ARockGenerator::ARockGenerator() : /*TrisReady(false),*/ VertsReady(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    SetRootComponent(StaticMeshComponent);
}

void ARockGenerator::BeginPlay()
{
	Super::BeginPlay();
}

void ARockGenerator::Tick(float DeltaTime) {}

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
	if (/*TrisReady.load() && */VertsReady.load()) {
		/*TrisReady.store(false);*/
		VertsReady.store(false);
		MeshGenerateCallback();
	}
}

void ARockGenerator::GenerateAndUpdateMesh()
{
	/*Tris = MakeShared<TArray<uint32>>();*/
	Verts = MakeShared<TArray<FVector3f>>();
	StartTime = MakeShared<FDateTime>(FDateTime::UtcNow());
	/*TrisReady.store(false);*/
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
	/*[this](const TArray<uint32>& InTris) {
		*Tris = InTris;
		TrisReady.store(true);
		CompleteCheckCallback();
	},*/
	[this](const TArray<FVector3f>& InVerts) {
		*Verts = InVerts;
		VertsReady.store(true);
		CompleteCheckCallback();
	},
	FBufferReadbackManager::GetInstance());
}

void ARockGenerator::MeshGenerateCallback() const
{
	const FDateTime EndTime = FDateTime::UtcNow();
	const float ExecutionTime = (EndTime - *StartTime).GetTotalSeconds();
	Debug::LogFloat(TEXT("Time Taken:"), ExecutionTime);
	int Length = 0;
	
	for (const auto Vert : *Verts)
	{
		if (Vert.X > std::numeric_limits<float>::max() - 1.f)
			break;
		Length++;
	}
		
	/*TArray<uint32> TrisCleaned;
	TrisCleaned.Append(*Tris);
	TrisCleaned.SetNum(Length);*/

	TArray<FVector3f> VertsCleaned;
	VertsCleaned.Append(*Verts);
	VertsCleaned.SetNum(Length);

	Debug::LogInt(TEXT("OriginalLength:"), Verts->Num());
	Debug::LogInt(TEXT("Length:"), Length);

	TArray<uint32> Tris;
	for (uint32 i = 0; i < static_cast<uint32>(Verts->Num()); ++i)
	{
		Tris.Add(i);
	}
		
	/*const auto StaticMesh = FStaticMeshGeneration::GenerateStaticMesh(SavePath, Name, Mat, VertsCleaned, /*TrisCleaned,#1# Tris);
	
	FStaticMeshLODResources& LODModel = StaticMesh->GetRenderData()->LODResources[0];

	FPositionVertexBuffer& PositionVertexBuffer = LODModel.VertexBuffers.PositionVertexBuffer;
	const FRawStaticIndexBuffer& IndexBuffer = LODModel.IndexBuffer;
	
	TArray<FVector3f> BakedVerts;
	TArray<uint32> BakedTris;
	
	for(uint32 Index = 0; Index < PositionVertexBuffer.GetNumVertices(); Index++)
	{
		FVector3f& VertexPosition = PositionVertexBuffer.VertexPosition(Index);
		BakedVerts.Add(VertexPosition);
	}

	for(int32 Index = 0; Index < IndexBuffer.GetNumIndices(); Index++)
	{
		const uint32 TriangleIndex = IndexBuffer.GetIndex(Index);
		BakedTris.Add(TriangleIndex);
	}
	
	const auto NaniteStaticMesh = FStaticMeshGeneration::GenerateStaticMesh(SavePath, Name, Mat, BakedVerts,
		BakedTris, true);*/

	const auto NaniteStaticMesh = FStaticMeshGeneration::GenerateStaticMesh(SavePath, Name, Mat, VertsCleaned,
		Tris, true);
	
	StaticMeshComponent->SetStaticMesh(NaniteStaticMesh);
}


