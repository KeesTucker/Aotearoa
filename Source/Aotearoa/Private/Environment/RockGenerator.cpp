// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/RockGenerator.h"

#include "Debug.h"
#include "RawMesh.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Environment/MarchingCubesUtility.h"
#include "Environment/VoxelGeneration.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ARockGenerator::ARockGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
}

// Called when the game starts or when spawned
void ARockGenerator::BeginPlay()
{
	Super::BeginPlay();

	const FString SavePath = "/Game/ProceduralMeshes/S_Generated";
	const FString Name = "S_Generated";
	
	if (RealTimeGeneration)
	{
		ProceduralMeshComponent->SetMaterial(0, Mat);
		return;
	}

	Resolution = Size * ResolutionPerUnit;
	Scale = Size / ResolutionPerUnit;
	
	auto Voxels = FVoxelGeneration::GenerateVoxelsWithNoise(Resolution, ShapeModifier, NoiseLayers);
	
	auto [Vertices, Triangles] = FMarchingCubesUtility::GenerateMesh(Resolution, Scale, Isolevel, Voxels);

	TArray<int32> FaceMatIndices;
	FaceMatIndices.Init(0, Triangles.Num() / 3);
	TArray<uint32> FaceSmoothingGroups;
	int32 DefaultSmoothingGroup = 0;
	DefaultSmoothingGroup |= 1;
	FaceSmoothingGroups.Init(DefaultSmoothingGroup, Triangles.Num() / 3);

	FRawMesh Mesh;
	
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
	StaticMeshComponent->SetStaticMesh(StaticMesh);
}

// Called every frame
void ARockGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!RealTimeGeneration) return;

	Resolution = FMath::RoundToInt(Size * ResolutionPerUnit);
	Scale = Size / ResolutionPerUnit;
	
	auto Voxels = FVoxelGeneration::GenerateVoxelsWithNoise(Resolution, ShapeModifier, NoiseLayers);
	
	auto [Vertices, Triangles] = FMarchingCubesUtility::GenerateMesh(Resolution, Scale, Isolevel, Voxels);

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
		FVector AverageNormal(0.0f, 0.0f, 0.0f);
		TArray<int32> TriangleIndices = VertexTriangleIndices[i];
		for (int j = 0; j < TriangleIndices.Num(); j += 3)
		{
			FVector Edge1 = ProceduralVertices[TriangleIndices[j + 2]] - ProceduralVertices[TriangleIndices[j]];
			FVector Edge2 = ProceduralVertices[TriangleIndices[j + 1]] - ProceduralVertices[TriangleIndices[j]];
			FVector Normal = FVector::CrossProduct(Edge1, Edge2);
			Normal.Normalize();
			AverageNormal += Normal;
		}
		AverageNormal.Normalize();
		Normals.Add(AverageNormal);
	}
	
	UVs.Init(FVector2d(0, 0), Vertices.Num());
	VertexColors.Init(FColor(0, 0, 0), Vertices.Num());
	Tangents.Init(FProcMeshTangent(0, 0, 0), Vertices.Num());

	ProceduralMeshComponent->CreateMeshSection(0, ProceduralVertices, ProceduralTriangles, Normals, UVs, VertexColors, Tangents, false);
}



