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


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UProceduralMeshComponent* ProceduralMesh;
};
