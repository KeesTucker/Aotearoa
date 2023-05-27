// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/RockGenerator.h"

#include "Debug.h"
#include "Environment/FastNoiseLite.h"
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
	constexpr int Size = 500;
	constexpr float PerlinScale = 2.5;
	constexpr float PerlinInfluence = 0.5;
	constexpr int Octaves = 3;
	
	TArray<TArray<TArray<float>>> Voxels = TArray<TArray<TArray<float>>>();
	TArray<FVector> Vertices = TArray<FVector>();
	TArray<TArray<int>> VertexTriangleIndices = TArray<TArray<int>>();
	TArray<int32> Triangles = TArray<int32>();
	TArray<FVector> Normals = TArray<FVector>();
	TArray<FVector2D> UV0 = TArray<FVector2D>();
	TArray<FLinearColor> VertexColors = TArray<FLinearColor>();
	TArray<FProcMeshTangent> Tangents = TArray<FProcMeshTangent>();

	FastNoiseLite FastNoise = FastNoiseLite();
	FastNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
	
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
					Noise += (FastNoise.GetNoise(x * PerlinScale * i, y * PerlinScale * i, z * PerlinScale * i) + 0.5f) * PerlinInfluence / i;
				}
				Voxels[x][y][z] = NormalizedDistance + Noise;
			}
		}
	}
	
	TMap<FString, int> VertexMap = TMap<FString, int>();
	
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
					int VertIndex1 = AddVertex(Voxels, Pos, Edges, i, Vertices, Triangles, VertexMap, Isolevel, VertexTriangleIndices);

					// Second edge lies between vertex e10 and vertex e11
					int VertIndex2 = AddVertex(Voxels, Pos, Edges, i + 1, Vertices, Triangles, VertexMap, Isolevel, VertexTriangleIndices);
        
					// Third edge lies between vertex e20 and vertex e21
					int VertIndex3 = AddVertex(Voxels, Pos, Edges, i + 2, Vertices, Triangles, VertexMap, Isolevel, VertexTriangleIndices);

					VertexColors.Add(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f));

					VertexTriangleIndices[VertIndex1].Add(VertIndex1);
					VertexTriangleIndices[VertIndex1].Add(VertIndex2);
					VertexTriangleIndices[VertIndex1].Add(VertIndex3);
					VertexTriangleIndices[VertIndex2].Add(VertIndex1);
					VertexTriangleIndices[VertIndex2].Add(VertIndex2);
					VertexTriangleIndices[VertIndex2].Add(VertIndex3);
					VertexTriangleIndices[VertIndex3].Add(VertIndex1);
					VertexTriangleIndices[VertIndex3].Add(VertIndex2);
					VertexTriangleIndices[VertIndex3].Add(VertIndex3);
				}
			}
		}
	}

	Normals.Reserve(Vertices.Num());

	for (int i = 0; i < Vertices.Num(); ++i)
	{
		FVector averageNormal(0.0f, 0.0f, 0.0f);

		TArray<int> triangleIndices = VertexTriangleIndices[i];
		for (int j = 0; j < triangleIndices.Num(); j += 3)
		{
			FVector edge1 = Vertices[triangleIndices[j + 2]] - Vertices[triangleIndices[j]];
			FVector edge2 = Vertices[triangleIndices[j + 1]] - Vertices[triangleIndices[j]];

			FVector normal = FVector::CrossProduct(edge1, edge2);
			normal.Normalize();

			averageNormal += normal;
		}

		averageNormal.Normalize();

		Normals.Add(averageNormal);
	}
	
	// Create the mesh section
	ProceduralMesh->ClearAllMeshSections();
	ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);
}

// Called every frame
void ARockGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

int ARockGenerator::AddVertex(const TArray<TArray<TArray<float>>>& Voxels, const FIntVector& Pos, const int* Edges,
                              const int EdgeIndex, TArray<FVector>& Vertices, TArray<int>& Triangles,
                              TMap<FString, int>& VertexMap,
                              const float Isolevel, TArray<TArray<int>>& VertexTriangleIndices)
{
	const int E00 = MarchingCubesLookupTables::EdgeConnections[Edges[EdgeIndex]][0];
	const int E01 = MarchingCubesLookupTables::EdgeConnections[Edges[EdgeIndex]][1];
	const FIntVector E00Index = Pos + MarchingCubesLookupTables::VertexOffsets[E00];
	const FIntVector E01Index = Pos + MarchingCubesLookupTables::VertexOffsets[E01];

	FString ID = MarchingCubesLookupTables::VertexOffsets[E00].ToString();
	ID += MarchingCubesLookupTables::VertexOffsets[E01].ToString();
	ID += Pos.ToString();
	FString UUID = LexToString(FMD5::HashAnsiString(*ID));

	int Index;
	if (VertexMap.Contains(UUID))
	{
		Index = VertexMap[UUID];
		Triangles.Add(Index);
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
		Index = Vertices.Num() - 1;
		VertexMap.Add(UUID, Index);
		Triangles.Add(Index);
		VertexTriangleIndices.Add(TArray<int>());
	}

	return Index;
}

