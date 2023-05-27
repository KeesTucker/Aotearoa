// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/RockGenerator.h"

#include "Debug.h"
#include "Environment/MarchingCubesLookupTables.h"

// Sets default values
ARockGenerator::ARockGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// Create Procedural Mesh Component and attach it to the root
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
}

// Called when the game starts or when spawned
void ARockGenerator::BeginPlay()
{
	Super::BeginPlay();

	constexpr float Isolevel = 0.5;
	constexpr int Size = 100;
	constexpr float PerlinScale = 0.1;
	constexpr float PerlinInfluence = 0.15;
	constexpr int Octaves = 3;
	
	TArray<TArray<TArray<float>>> Voxels;
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	Voxels.SetNum(Size);
	for (int x = 0; x < Size; ++x)
	{
		Voxels[x].SetNum(Size);
		for (int y = 0; y < Size; ++y)
		{
			Voxels[x][y].SetNum(Size);
			for (int z = 0; z < Size; ++z)
			{
				float Distance = FVector::Dist(FVector(x, y, z), FVector(Size / 2, Size / 2, Size / 2));
				float NormalizedDistance = 1.f - Distance / (Size / 2);
				NormalizedDistance = FMath::Clamp(NormalizedDistance, 0.f, 1.f);
				float Noise = 0;
				for (float i = 1.f; i <= Octaves; ++i)
				{
					Noise += FMath::PerlinNoise3D(FVector(x, y, z) * PerlinScale * i) * PerlinInfluence / i;
				}
				
				Voxels[x][y][z] = NormalizedDistance + Noise;
			}
		}
	}

	TMap<FIntVector, int> VertexMap = TMap<FIntVector, int>();
	
	for (int x = 0; x < Size - 1; ++x)
	{
		for (int y = 0; y < Size - 1; ++y)
		{
			for (int z = 0; z < Size - 1; ++z)
			{
				FIntVector Pos = FIntVector(x, y, z);
				int CubeIndex = 0;

				FIntVector VertexPos = Pos + MarchingCubesLookupTables::VertexOffsets[0];
				if (Voxels[VertexPos.X][VertexPos.Y][VertexPos.Z] < Isolevel) CubeIndex |= 1;
				VertexPos = Pos + MarchingCubesLookupTables::VertexOffsets[1];
				if (Voxels[VertexPos.X][VertexPos.Y][VertexPos.Z] < Isolevel) CubeIndex |= 2;
				VertexPos = Pos + MarchingCubesLookupTables::VertexOffsets[2];
				if (Voxels[VertexPos.X][VertexPos.Y][VertexPos.Z] < Isolevel) CubeIndex |= 4;
				VertexPos = Pos + MarchingCubesLookupTables::VertexOffsets[3];
				if (Voxels[VertexPos.X][VertexPos.Y][VertexPos.Z] < Isolevel) CubeIndex |= 8;
				VertexPos = Pos + MarchingCubesLookupTables::VertexOffsets[4];
				if (Voxels[VertexPos.X][VertexPos.Y][VertexPos.Z] < Isolevel) CubeIndex |= 16;
				VertexPos = Pos + MarchingCubesLookupTables::VertexOffsets[5];
				if (Voxels[VertexPos.X][VertexPos.Y][VertexPos.Z] < Isolevel) CubeIndex |= 32;
				VertexPos = Pos + MarchingCubesLookupTables::VertexOffsets[6];
				if (Voxels[VertexPos.X][VertexPos.Y][VertexPos.Z] < Isolevel) CubeIndex |= 64;
				VertexPos = Pos + MarchingCubesLookupTables::VertexOffsets[7];
				if (Voxels[VertexPos.X][VertexPos.Y][VertexPos.Z] < Isolevel) CubeIndex |= 128;
				
				const int* Edges = MarchingCubesLookupTables::TriTable[CubeIndex];
				
				for (int i = 0; Edges[i] != -1; i += 3)
				{
					const int VertexIndex = Vertices.Num();
					
					Triangles.Add(VertexIndex + 1);
					Triangles.Add(VertexIndex + 2);
					
					// First edge lies between vertex e00 and vertex e01
					AddEdge(Voxels, Pos, Edges, i, Vertices, Triangles, VertexMap, Isolevel);

					// Second edge lies between vertex e10 and vertex e11
					AddEdge(Voxels, Pos, Edges, i + 1, Vertices, Triangles, VertexMap, Isolevel);
        
					// Third edge lies between vertex e20 and vertex e21
					AddEdge(Voxels, Pos, Edges, i + 2, Vertices, Triangles, VertexMap, Isolevel);
				}
			}
		}
	}

	// Create the mesh section
	ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);
}

void ARockGenerator::AddEdge(const TArray<TArray<TArray<float>>>& Voxels, const FIntVector& Pos, const int* Edges, const int EdgeIndex, TArray<FVector>& Vertices, TArray<int>& Triangles, TMap<FIntVector, int>& VertexMap, float Isolevel)
{
	const int E00 = MarchingCubesLookupTables::EdgeConnections[Edges[EdgeIndex]][0];
	const int E01 = MarchingCubesLookupTables::EdgeConnections[Edges[EdgeIndex]][1];
	const FIntVector E00Index = Pos + MarchingCubesLookupTables::VertexOffsets[E00];
	const FIntVector E01Index = Pos + MarchingCubesLookupTables::VertexOffsets[E01];
	if (const FIntVector ID = (MarchingCubesLookupTables::VertexOffsets[E00] + MarchingCubesLookupTables::VertexOffsets[E01]) / 2 + Pos;
		VertexMap.Contains(ID))
	{
		Triangles.Add(VertexMap[ID]);
	}
	else
	{
		Vertices.Add(
		MarchingCubesLookupTables::Interp(
				FVector(MarchingCubesLookupTables::VertexOffsets[E00]),
				Voxels[E00Index.X][E00Index.Y][E00Index.Z],
				FVector(MarchingCubesLookupTables::VertexOffsets[E01]),
				Voxels[E01Index.X][E01Index.Y][E01Index.Z],
				Isolevel
			) + FVector(Pos));
		const int Index = Vertices.Num() - 1;
		VertexMap.Add(ID, Index);
		Triangles.Add(Index);
	}
}

// Called every frame
void ARockGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

