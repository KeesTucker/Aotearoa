// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ProceduralMeshComponent.h"

#include "CoreMinimal.h"
#include "VoxelGeneration.h"
#include "GameFramework/Actor.h"
#include "RockGenerator.generated.h"

UCLASS()
class AOTEAROA_API ARockGenerator : public AActor
{
	GENERATED_BODY()
	
public:
	explicit ARockGenerator();

	UPROPERTY(EditAnywhere)
	bool RealTimeGeneration = false;
	
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

	int Resolution = Size * ResolutionPerUnit;
	float Scale = Size / ResolutionPerUnit;
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UProceduralMeshComponent* ProceduralMeshComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* StaticMeshComponent;
};
