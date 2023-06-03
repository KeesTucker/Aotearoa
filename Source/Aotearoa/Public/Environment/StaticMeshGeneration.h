#pragma once
#include "AssetRegistry/AssetRegistryModule.h"
#include "Debug.h"
#include "RawMesh.h"
#include "UObject/SavePackage.h"

class FStaticMeshGeneration
{
public:
	static UStaticMesh* GenerateStaticMesh(const FString& SavePath, const FString& Name,
	                                       const TArray<FVector3f>& Vertices, const TArray<uint32>& Triangles)
	{
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
		
		UPackage* Package = CreatePackage(*(SavePath + Name));
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
		//StaticMesh->GetStaticMaterials().Add(FStaticMaterial(Mat));
		//Set the Imported version before calling the build
		StaticMesh->ImportVersion = LastVersion;
		StaticMesh->NaniteSettings.bEnabled = true;
		// Build mesh from source
		StaticMesh->Build(false);
		
		StaticMesh->PostEditChange();
	
		// Notify asset registry of new asset
		FAssetRegistryModule::AssetCreated(StaticMesh);
	
		if (!StaticMesh->MarkPackageDirty())
		{
			Debug::LogError(TEXT("Could not mark mesh as dirty"));
			return StaticMesh;
		}
		
		// Define the save parameters
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

		const FString PackageFileName = FPackageName::LongPackageNameToFilename(*(SavePath + Name), FPackageName::GetAssetPackageExtension());
		
		// Save the package
		if (!UPackage::SavePackage(Package, StaticMesh, *PackageFileName, SaveArgs))
		{
			Debug::LogError(TEXT("Could not save mesh"));
			return StaticMesh;
		}
		Debug::LogFString(TEXT("Saved mesh succesfully:"), PackageFileName);
		return StaticMesh;
	}
};
