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
	RootComponent = ProceduralMesh;
}

// Called when the game starts or when spawned
void ARockGenerator::BeginPlay()
{
	Super::BeginPlay();

	constexpr float Isolevel = 0.5;
	constexpr int Size = 10;
	
	TArray<float> Voxels;
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	for (int x = 0; x < Size; ++x)
	{
		for (int y = 0; y < Size; ++y)
		{
			for (int z = 0; z < Size; ++z)
			{
				if (x == 0 || y == 0 || z == 0 || x == Size - 1 || y == Size - 1 || z == Size - 1)
				{
					Voxels.Add(0);
				}
				else
				{
					Voxels.Add(1.0);
				}
			}
		}
	}
	
	for (int x = 0; x < Size - 1; ++x)
	{
		for (int y = 0; y < Size - 1; ++y)
		{
			for (int z = 0; z < Size - 1; ++z)
			{
				const int PositionalIndex = MarchingCubesLookupTables::GetPositionIndex(FIntVector(x, y, z), Size);
				
				int CubeIndex = 0;
				
				if (Voxels[PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(0, Size)] < Isolevel) CubeIndex |= 1;
				if (Voxels[PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(1, Size)] < Isolevel) CubeIndex |= 2;
				if (Voxels[PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(2, Size)] < Isolevel) CubeIndex |= 4;
				if (Voxels[PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(3, Size)] < Isolevel) CubeIndex |= 8;
				if (Voxels[PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(4, Size)] < Isolevel) CubeIndex |= 16;
				if (Voxels[PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(5, Size)] < Isolevel) CubeIndex |= 32;
				if (Voxels[PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(6, Size)] < Isolevel) CubeIndex |= 64;
				if (Voxels[PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(7, Size)] < Isolevel) CubeIndex |= 128;

				Debug::LogInt(TEXT("CubeIndex:"), CubeIndex);
				
				const int* Edges = MarchingCubesLookupTables::GTriTable[CubeIndex];

				for (int i = 0; i < 16; ++i)
				{
					Debug::LogInt(TEXT("Edge:"), Edges[i]);
				}


				for (int i = 0; Edges[i] != -1; i += 3)
				{
					// First edge lies between vertex e00 and vertex e01
					const int E00 = MarchingCubesLookupTables::GEdgeConnections[Edges[i]][0];
					const int E01 = MarchingCubesLookupTables::GEdgeConnections[Edges[i]][1];
					const int E00Index = PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(E00, Size);
					const int E01Index = PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(E01, Size);
					Vertices.Add(
					MarchingCubesLookupTables::Interp(
							FVector(MarchingCubesLookupTables::GCornerOffsets[E00]),
							Voxels[E00Index],
							FVector(MarchingCubesLookupTables::GCornerOffsets[E01]),
							Voxels[E01Index],
							Isolevel
						) + FVector(MarchingCubesLookupTables::GetPositionVector(E00Index, Size))
					);

					// Second edge lies between vertex e10 and vertex e11
					const int E10 = MarchingCubesLookupTables::GEdgeConnections[Edges[i + 1]][0];
					const int E11 = MarchingCubesLookupTables::GEdgeConnections[Edges[i + 1]][1];
					const int E10Index = PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(E00, Size);
					const int E11Index = PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(E01, Size);
					Vertices.Add(
					MarchingCubesLookupTables::Interp(
							FVector(MarchingCubesLookupTables::GCornerOffsets[E10]),
							Voxels[E10Index],
							FVector(MarchingCubesLookupTables::GCornerOffsets[E11]),
							Voxels[E11Index],
							Isolevel
						) + FVector(MarchingCubesLookupTables::GetPositionVector(E10Index, Size))
					);
        
					// Third edge lies between vertex e20 and vertex e21
					const int E20 = MarchingCubesLookupTables::GEdgeConnections[Edges[i + 2]][0];
					const int E21 = MarchingCubesLookupTables::GEdgeConnections[Edges[i + 2]][1];
					const int E20Index = PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(E20, Size);
					const int E21Index = PositionalIndex + MarchingCubesLookupTables::GetPositionOffset(E21, Size);
					Vertices.Add(
					MarchingCubesLookupTables::Interp(
							FVector(MarchingCubesLookupTables::GCornerOffsets[E20]),
							Voxels[E20Index],
							FVector(MarchingCubesLookupTables::GCornerOffsets[E21]),
							Voxels[E21Index],
							Isolevel
						) + FVector(MarchingCubesLookupTables::GetPositionVector(E20Index, Size))
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

