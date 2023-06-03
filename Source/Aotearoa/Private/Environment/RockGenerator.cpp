// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/RockGenerator.h"

#include "Debug.h"
#include "MeshBuild.h"
#include "RawMesh.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Environment/FastNoiseLite.h"
#include "Environment/MarchingCubesLookupTables.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ARockGenerator::ARockGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
}

// Called when the game starts or when spawned
void ARockGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	FString SavePath = "/Game/ProceduralMeshes/S_Generated";
	FString Name = "S_Generated";
	
	if (RealTimeGeneration)
	{
		ProceduralMeshComponent->SetMaterial(0, Mat);
		return;
	}

	Resolution = Size * ResolutionPerUnit;
	Scale = Size / ResolutionPerUnit;
	
	TArray<TArray<TArray<float>>> Voxels = TArray<TArray<TArray<float>>>();
	TArray<FVector3f> Vertices = TArray<FVector3f>();
	TArray<uint32> Triangles = TArray<uint32>();
	FRawMesh Mesh;

	FastNoiseLite CellularNoise = FastNoiseLite();
	CellularNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
	FastNoiseLite PerlinNoise = FastNoiseLite();
	PerlinNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	
	Voxels.SetNum(Resolution);
	for (int x = 0; x < Resolution; ++x)
	{
		Voxels[x].SetNum(Resolution);
		for (int y = 0; y < Resolution; ++y)
		{
			Voxels[x][y].SetNum(Resolution);
			for (int z = 0; z < Resolution; ++z)
			{
				float Distance = FVector3f::Dist(FVector3f(x, y, z), FVector3f(Resolution / 2, Resolution / 2, Resolution / 2));
				float NormalizedDistance = 1.f - Distance / (Resolution / 2);
				NormalizedDistance = FMath::Clamp(NormalizedDistance, 0.f, 1.f);
				float Noise = 0;
				for (float i = 1.f; i <= Octaves; ++i)
				{
					Noise += (CellularNoise.GetNoise(x * PerlinScale * Scale * i, y * PerlinScale * Scale * i, z * PerlinScale * Scale * i) + 0.5f) * PerlinInfluence / i;
				}
				Voxels[x][y][z] = NormalizedDistance + Noise;
			}
		}
	}
	
	TMap<FString, int> VertexMap = TMap<FString, int>();
	
	for (int x = 0; x < Resolution - 1; ++x)
	{
		for (int y = 0; y < Resolution - 1; ++y)
		{
			for (int z = 0; z < Resolution - 1; ++z)
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
					FVector3f ObjectOriginOffset = FVector3f(Resolution / 2, Resolution / 2, Resolution / 2);
					FVector3f VectorPos = FVector3f(Pos) - ObjectOriginOffset;
					
					// First edge lies between vertex e00 and vertex e01
					AddVertex(Voxels, Pos, VectorPos, Edges, i, Vertices, Triangles, VertexMap);

					// Second edge lies between vertex e10 and vertex e11
					AddVertex(Voxels, Pos, VectorPos, Edges, i + 1, Vertices, Triangles, VertexMap);
        
					// Third edge lies between vertex e20 and vertex e21
					AddVertex(Voxels, Pos, VectorPos, Edges, i + 2, Vertices, Triangles, VertexMap);
				}
			}
		}
	}

	TArray<int32> FaceMatIndices;
	FaceMatIndices.Init(0, Triangles.Num() / 3);
	TArray<uint32> FaceSmoothingGroups;
	int32 DefaultSmoothingGroup = 0;
	DefaultSmoothingGroup |= 1;
	FaceSmoothingGroups.Init(DefaultSmoothingGroup, Triangles.Num() / 3);

	Mesh.VertexPositions = Vertices;
	Mesh.WedgeIndices = Triangles;
	Mesh.FaceMaterialIndices = FaceMatIndices;
	Mesh.FaceSmoothingMasks = FaceSmoothingGroups;
	Mesh.WedgeTexCoords[0].Init(FVector2f(0.0f, 0.0f), Triangles.Num());
	Mesh.WedgeColors.Init(FColor(0, 0, 0, 0), Triangles.Num());
	Mesh.WedgeTangentY.Init(FVector3f(0.0f, 0.0f, 1.0f), Triangles.Num());

	UPackage* Package = CreatePackage(*SavePath);
	check(Package);
	// Create StaticMesh object
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, FName(*Name), RF_Public | RF_Standalone);
	StaticMesh->InitResources();
	//FGuid::NewGuid() = StaticMesh->GetLightingGuid();
	//StaticMesh->GetLightingGuid() = FGuid::NewGuid();
	// Create a Source Model then set it to variable
	FStaticMeshSourceModel& SrcModel = StaticMesh->AddSourceModel();;
	// Add source to new StaticMesh
	SrcModel.BuildSettings.bRecomputeNormals = true;
	SrcModel.BuildSettings.bRecomputeTangents = true;
	SrcModel.BuildSettings.bRemoveDegenerates = false;
	SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
	SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
	SrcModel.BuildSettings.bGenerateLightmapUVs = true;
	SrcModel.BuildSettings.SrcLightmapIndex = 0;
	SrcModel.BuildSettings.DstLightmapIndex = 1;
	SrcModel.RawMeshBulkData->SaveRawMesh(Mesh);
	// Copy materials to new mesh
	StaticMesh->GetStaticMaterials().Add(FStaticMaterial(Mat));
	//Set the Imported version before calling the build
	StaticMesh->ImportVersion = LastVersion;
	StaticMesh->NaniteSettings.bEnabled = true;
	// Build mesh from source
	StaticMesh->Build(false);

	StaticMesh->PostEditChange();
	// Notify asset registry of new asset
	FAssetRegistryModule::AssetCreated(StaticMesh);
}

// Called every frame
void ARockGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!RealTimeGeneration)
	{
		return;
	}

	Resolution = FMath::RoundToInt(Size * ResolutionPerUnit);
	Scale = Size / ResolutionPerUnit;

	Debug::LogInt(TEXT(""), Resolution);
	Debug::LogFloat(TEXT(""), Scale);
	
	TArray<TArray<TArray<float>>> Voxels = TArray<TArray<TArray<float>>>();
	TArray<FVector3f> Vertices = TArray<FVector3f>();
	TArray<uint32> Triangles = TArray<uint32>();

	FastNoiseLite CellularNoise = FastNoiseLite();
	CellularNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
	FastNoiseLite PerlinNoise = FastNoiseLite();
	PerlinNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	
	Voxels.SetNum(Resolution);
	for (int x = 0; x < Resolution; ++x)
	{
		Voxels[x].SetNum(Resolution);
		for (int y = 0; y < Resolution; ++y)
		{
			Voxels[x][y].SetNum(Resolution);
			for (int z = 0; z < Resolution; ++z)
			{
				float Distance = FVector3f::Dist(FVector3f(x, y, z), FVector3f(Resolution / 2, Resolution / 2, Resolution / 2));
				float NormalizedDistance = 1.f - Distance / (Resolution / 2);
				NormalizedDistance = FMath::Clamp(NormalizedDistance, 0.f, 1.f);
				float Noise = 0;
				for (float i = 1.f; i <= Octaves; ++i)
				{
					Noise += (CellularNoise.GetNoise(x * PerlinScale / Resolution * i, y * PerlinScale / Resolution * i, z * PerlinScale / Resolution * i) + 0.5f) * PerlinInfluence / i;
				}
				Voxels[x][y][z] = NormalizedDistance + Noise;
			}
		}
	}
	
	TMap<FString, int> VertexMap = TMap<FString, int>();
	
	for (int x = 0; x < Resolution - 1; ++x)
	{
		for (int y = 0; y < Resolution - 1; ++y)
		{
			for (int z = 0; z < Resolution - 1; ++z)
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
					FVector3f ObjectOriginOffset = FVector3f(Resolution / 2, Resolution / 2, Resolution / 2);
					FVector3f VectorPos = FVector3f(Pos) - ObjectOriginOffset;
					
					// First edge lies between vertex e00 and vertex e01
					AddVertex(Voxels, Pos, VectorPos, Edges, i, Vertices, Triangles, VertexMap);

					// Second edge lies between vertex e10 and vertex e11
					AddVertex(Voxels, Pos, VectorPos, Edges, i + 1, Vertices, Triangles, VertexMap);
        
					// Third edge lies between vertex e20 and vertex e21
					AddVertex(Voxels, Pos, VectorPos, Edges, i + 2, Vertices, Triangles, VertexMap);
				}
			}
		}
	}

	TArray<FVector> ProceduralVertices;
	TArray<int32> ProceduralTriangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	ProceduralVertices.SetNum(Vertices.Num());
	for (int i = 0; i < Vertices.Num(); ++i)
	{
		ProceduralVertices[i] = FVector(Vertices[i]);
	}

	ProceduralTriangles.SetNum(Triangles.Num());
	for (int i = 0; i < Triangles.Num(); ++i)
	{
		ProceduralTriangles[i] = static_cast<int32>(Triangles[i]);
	}

	TArray<TArray<int32>> VertexTriangleIndices;
	VertexTriangleIndices.Init(TArray<int32>(), Vertices.Num());
	for (int i = 0; i < ProceduralTriangles.Num(); i += 3)
	{
		for (int j = 0; j < 3; ++j)
		{
			for (int k = 0; k < 3; ++k)
			{
				VertexTriangleIndices[ProceduralTriangles[i + j]].Add(ProceduralTriangles[i + k]);
			}
		}
	}
	
	Normals.Reserve(Vertices.Num());
	for (int i = 0; i < Vertices.Num(); ++i)
	{
		FVector averageNormal(0.0f, 0.0f, 0.0f);
		TArray<int32> triangleIndices = VertexTriangleIndices[i];
		for (int j = 0; j < triangleIndices.Num(); j += 3)
		{
			FVector edge1 = ProceduralVertices[triangleIndices[j + 2]] - ProceduralVertices[triangleIndices[j]];
			FVector edge2 = ProceduralVertices[triangleIndices[j + 1]] - ProceduralVertices[triangleIndices[j]];
			FVector normal = FVector::CrossProduct(edge1, edge2);
			normal.Normalize();
			averageNormal += normal;
		}
		averageNormal.Normalize();
		Normals.Add(averageNormal);
	}
	
	UVs.Init(FVector2d(0, 0), Vertices.Num());
	VertexColors.Init(FColor(0, 0, 0), Vertices.Num());
	Tangents.Init(FProcMeshTangent(0, 0, 0), Vertices.Num());

	ProceduralMeshComponent->CreateMeshSection(0, ProceduralVertices, ProceduralTriangles, Normals, UVs, VertexColors, Tangents, false);
}

void ARockGenerator::AddVertex(const TArray<TArray<TArray<float>>>& Voxels, const FIntVector& VoxelPos, const FVector3f& WorldPos, const int* Edges,
                               const int EdgeIndex, TArray<FVector3f>& Vertices, TArray<uint32>& Triangles, TMap<FString,
                               int>& VertexMap)
{
	const int E00 = MarchingCubesLookupTables::EdgeConnections[Edges[EdgeIndex]][0];
	const int E01 = MarchingCubesLookupTables::EdgeConnections[Edges[EdgeIndex]][1];
	const FIntVector E00Index = VoxelPos + MarchingCubesLookupTables::VertexOffsets[E00];
	const FIntVector E01Index = VoxelPos + MarchingCubesLookupTables::VertexOffsets[E01];

	FString ID = MarchingCubesLookupTables::VertexOffsets[E00].ToString();
	ID += MarchingCubesLookupTables::VertexOffsets[E01].ToString();
	ID += VoxelPos.ToString();
	FString UUID = LexToString(FMD5::HashAnsiString(*ID));

	int Index;
	if (VertexMap.Contains(UUID))
	{
		Index = VertexMap[UUID];
		Triangles.Add(Index);
	}
	else
	{
		FVector3f Vert = MarchingCubesLookupTables::Interp(
				FVector3f(MarchingCubesLookupTables::VertexOffsets[E00]),
				Voxels[E00Index.X][E00Index.Y][E00Index.Z],
				FVector3f(MarchingCubesLookupTables::VertexOffsets[E01]),
				Voxels[E01Index.X][E01Index.Y][E01Index.Z],
				Isolevel
			) + WorldPos;
		Vert = Vert * Scale;
		Vertices.Add(Vert);
		Index = Vertices.Num() - 1;
		VertexMap.Add(UUID, Index);
		Triangles.Add(Index);
	}
}

