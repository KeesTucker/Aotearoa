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
				float Noise = FMath::PerlinNoise3D(FVector(x, y, z) * PerlinScale) * PerlinInfluence;
				Voxels[x][y][z] = NormalizedDistance + Noise;
			}
		}
	}
	
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
					// First edge lies between vertex e00 and vertex e01
					const int E00 = MarchingCubesLookupTables::EdgeConnections[Edges[i]][0];
					const int E01 = MarchingCubesLookupTables::EdgeConnections[Edges[i]][1];
					const FIntVector E00Index = Pos + MarchingCubesLookupTables::VertexOffsets[E00];
					const FIntVector E01Index = Pos + MarchingCubesLookupTables::VertexOffsets[E01];
					Vertices.Add(
					MarchingCubesLookupTables::Interp(
							FVector(MarchingCubesLookupTables::VertexOffsets[E00]),
							Voxels[E00Index.X][E00Index.Y][E00Index.Z],
							FVector(MarchingCubesLookupTables::VertexOffsets[E01]),
							Voxels[E01Index.X][E01Index.Y][E01Index.Z],
							Isolevel
						) + FVector(Pos)
					);

					// Second edge lies between vertex e10 and vertex e11
					const int E10 = MarchingCubesLookupTables::EdgeConnections[Edges[i + 1]][0];
					const int E11 = MarchingCubesLookupTables::EdgeConnections[Edges[i + 1]][1];
					const FIntVector E10Index = Pos + MarchingCubesLookupTables::VertexOffsets[E10];
					const FIntVector E11Index = Pos + MarchingCubesLookupTables::VertexOffsets[E11];
					Vertices.Add(
					MarchingCubesLookupTables::Interp(
							FVector(MarchingCubesLookupTables::VertexOffsets[E10]),
							Voxels[E10Index.X][E10Index.Y][E10Index.Z],
							FVector(MarchingCubesLookupTables::VertexOffsets[E11]),
							Voxels[E11Index.X][E11Index.Y][E11Index.Z],
							Isolevel
						) + FVector(Pos)
					);
        
					// Third edge lies between vertex e20 and vertex e21
					const int E20 = MarchingCubesLookupTables::EdgeConnections[Edges[i + 2]][0];
					const int E21 = MarchingCubesLookupTables::EdgeConnections[Edges[i + 2]][1];
					const FIntVector E20Index = Pos + MarchingCubesLookupTables::VertexOffsets[E20];
					const FIntVector E21Index = Pos + MarchingCubesLookupTables::VertexOffsets[E21];
					Vertices.Add(
					MarchingCubesLookupTables::Interp(
							FVector(MarchingCubesLookupTables::VertexOffsets[E20]),
							Voxels[E20Index.X][E20Index.Y][E20Index.Z],
							FVector(MarchingCubesLookupTables::VertexOffsets[E21]),
							Voxels[E21Index.X][E21Index.Y][E21Index.Z],
							Isolevel
						) + FVector(Pos)
					);

					const int VertexIndex = Vertices.Num() - 3;
					Triangles.Add(VertexIndex);
					Triangles.Add(VertexIndex + 1);
					Triangles.Add(VertexIndex + 2);
				}
			}
		}
	}

	// Create the mesh section
	ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);
}

// Called every frame
void ARockGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

