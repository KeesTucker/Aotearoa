// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VoxelGeneration.h"
#include "GameFramework/Actor.h"
#include "RockGenerator.generated.h"

#define NUM_THREADS_VOXEL_DENSITY_COMPUTE_SHADER 8 //TODO: use the one from VoxelDensityComputeShader

UCLASS()
class AOTEAROA_API ARockGenerator : public AActor
{
	GENERATED_BODY()
	
public:
	explicit ARockGenerator();

	UPROPERTY(EditAnywhere)
	bool RegenOnEdit = true;

	UPROPERTY(EditAnywhere)
	FString SavePath = TEXT("/Game/ProceduralMeshes/");
	UPROPERTY(EditAnywhere)
	FString Name = TEXT("S_Generated");

	UPROPERTY(EditAnywhere)
	float Seed = 0.f;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* Mat;
	
	UPROPERTY(EditAnywhere)
	float Size = 10.0f;
	UPROPERTY(EditAnywhere)
	float ResolutionPerUnit = 2.0f;
	UPROPERTY(EditAnywhere)
	float Isolevel = 0.5f;
	UPROPERTY(EditAnywhere)
	EShapeModifier ShapeModifier = EShapeModifier::EShapeModifier_Sphere;
	UPROPERTY(EditAnywhere)
	TArray<FNoiseLayer> NoiseLayers;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* StaticMeshComponent;

private:
	FBufferReadbackManager ReadbackManager;
	
	TSharedPtr<TArray<uint32>> Tris;
	TSharedPtr<TArray<FVector3f>> Verts;
	TSharedPtr<FDateTime> StartTime;
	std::atomic<bool> TrisReady;
	std::atomic<bool> VertsReady;

	void GenerateAndUpdateMesh();
	void MeshGenerateCallback() const;
	void CompleteCheckCallback();
};
