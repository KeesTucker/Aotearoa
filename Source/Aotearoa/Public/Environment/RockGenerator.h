// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ProceduralMeshComponent.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RockGenerator.generated.h"

UCLASS()
class AOTEAROA_API ARockGenerator : public AActor
{
	GENERATED_BODY()
	
public:
	ARockGenerator();

	UPROPERTY(EditAnywhere)
	bool RealTimeGeneration = false;
	
	UPROPERTY(EditAnywhere)
	UMaterialInterface* Mat;
	
	UPROPERTY(EditAnywhere)
	float Isolevel = 0.5f;
	UPROPERTY(EditAnywhere)
	float Size = 5.0f;
	UPROPERTY(EditAnywhere)
	float ResolutionPerUnit = 1.0f;
	UPROPERTY(EditAnywhere)
	float PerlinScale = 200.0f;
	UPROPERTY(EditAnywhere)
	float PerlinInfluence = 0.5f;
	UPROPERTY(EditAnywhere)
	int Octaves = 3;

	int Resolution = Size * ResolutionPerUnit;
	float Scale = Size / ResolutionPerUnit;
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void AddVertex(const TArray<TArray<TArray<float>>>& Voxels, const FIntVector& VoxelPos, const FVector3f& WorldPos,
	                       const int* Edges, const int EdgeIndex, TArray<FVector3f>& Vertices,
	                       TArray<uint32>& Triangles, TMap<FString, int>& VertexMap);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UProceduralMeshComponent* ProceduralMeshComponent;
};
