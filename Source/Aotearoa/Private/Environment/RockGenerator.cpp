// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/RockGenerator.h"

#include "Debug.h"
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
	
	// Create Procedural Mesh Component and attach it to the root
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Mats/MI_Rock.MI_Rock'"));
	if (MaterialAsset.Succeeded())
	{
		RockMat = MaterialAsset.Object;
	}
}

// Called when the game starts or when spawned
void ARockGenerator::BeginPlay()
{
	Super::BeginPlay();

	constexpr float Isolevel = 0.5;
	constexpr int Size = 100;
	constexpr float PerlinScale = 2.5;
	constexpr float PerlinInfluence = 0.5;
	constexpr int Octaves = 3;
	FString SavePath = "/Game/ProceduralMeshes";
	FString Name = "Rock";
	
	TArray<TArray<TArray<float>>> Voxels = TArray<TArray<TArray<float>>>();
	TArray<FVector3f> Vertices = TArray<FVector3f>();
	TArray<uint32> Triangles = TArray<uint32>();
	FRawMesh Mesh;

	FastNoiseLite CellularNoise = FastNoiseLite();
	CellularNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
	FastNoiseLite PerlinNoise = FastNoiseLite();
	PerlinNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	
	Voxels.SetNum(Size);
	for (int x = 0; x < Size; ++x)
	{
		Voxels[x].SetNum(Size);
		for (int y = 0; y < Size; ++y)
		{
			Voxels[x][y].SetNum(Size);
			for (int z = 0; z < Size; ++z)
			{
				float Distance = FVector3f::Dist(FVector3f(x, y, z), FVector3f(Size / 2, Size / 2, Size / 2));
				float NormalizedDistance = 1.f - Distance / (Size / 2);
				NormalizedDistance = FMath::Clamp(NormalizedDistance, 0.f, 1.f);
				float Noise = 0;
				for (float i = 1.f; i <= Octaves; ++i)
				{
					Noise += (CellularNoise.GetNoise(x * PerlinScale * i, y * PerlinScale * i, z * PerlinScale * i) + 0.5f) * PerlinInfluence / i;
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
					AddVertex(Voxels, Pos, Edges, i, Vertices, Triangles, VertexMap, Isolevel);

					// Second edge lies between vertex e10 and vertex e11
					AddVertex(Voxels, Pos, Edges, i + 1, Vertices, Triangles, VertexMap, Isolevel);
        
					// Third edge lies between vertex e20 and vertex e21
					AddVertex(Voxels, Pos, Edges, i + 2, Vertices, Triangles, VertexMap, Isolevel);
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
	StaticMesh->GetStaticMaterials().Add(FStaticMaterial(RockMat));
	//Set the Imported version before calling the build
	StaticMesh->ImportVersion = LastVersion;
	StaticMesh->NaniteSettings.bEnabled = true;
	// Build mesh from source
	StaticMesh->Build(false);

	StaticMesh->PostEditChange();
	// Notify asset registry of new asset
	FAssetRegistryModule::AssetCreated(StaticMesh);

	StaticMeshComponent->SetStaticMesh(StaticMesh);
}

// Called every frame
void ARockGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ARockGenerator::AddVertex(const TArray<TArray<TArray<float>>>& Voxels, const FIntVector& Pos, const int* Edges,
                               const int EdgeIndex, TArray<FVector3f>& Vertices, TArray<uint32>& Triangles, TMap<FString,
                               int>& VertexMap, const float Isolevel)
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
		const FVector3f Vert = MarchingCubesLookupTables::Interp(
				FVector3f(MarchingCubesLookupTables::VertexOffsets[E00]),
				Voxels[E00Index.X][E00Index.Y][E00Index.Z],
				FVector3f(MarchingCubesLookupTables::VertexOffsets[E01]),
				Voxels[E01Index.X][E01Index.Y][E01Index.Z],
				Isolevel
			) + FVector3f(Pos);
		Vertices.Add(Vert);
		Index = Vertices.Num() - 1;
		VertexMap.Add(UUID, Index);
		Triangles.Add(Index);
	}
}

