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
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void AddEdge(const TArray<TArray<TArray<float>>>& Voxels, const FIntVector& Pos, const int* Edges, const int EdgeIndex, TArray<FVector>& Vertices, TArray<int>& Triangles, TMap<FString, int>& VertexMap, float Isolevel);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UProceduralMeshComponent* ProceduralMesh;
};
