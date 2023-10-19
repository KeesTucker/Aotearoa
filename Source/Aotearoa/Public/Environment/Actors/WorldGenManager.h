#pragma once

#include "CoreMinimal.h"
#include "RockGenerator.h"
#include "GameFramework/Actor.h"
#include "WorldGenManager.generated.h"

class ALandscape;

UCLASS()
class AOTEAROA_API AWorldGenManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AWorldGenManager();

	UPROPERTY(EditAnywhere)
	bool B_Generate = false;
	UPROPERTY(EditAnywhere, Category = "Rock Spawning")
	bool B_PlaceRocks = false;
	UPROPERTY(EditAnywhere, Category = "Macro Spawning")
	bool B_PlaceMacro = false;

	UPROPERTY(EditAnywhere)
	float Size = 1024.f;
	UPROPERTY(EditAnywhere)
	int ResolutionPerChunk = 256;
	UPROPERTY(EditAnywhere)
	float ResolutionPerUnit = 1.f;
	UPROPERTY(EditAnywhere)
	ARockGenerator* TemplateGenerator = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ALandscape* Landscape = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SpawnDensity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SpawnHeight = -200;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float PerlinSize = 0.001;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float NoSpawnChance = 0.3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MinScale = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxScale = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxPosOffset = 1000;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxHeightOffset = 500;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<UStaticMesh*> Rocks = TArray<UStaticMesh*>();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Macro Spawning")
	TSubclassOf<class AMacroKelp> MacroBP;
	
protected:
	virtual void BeginPlay() override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;

private:
	void GenerateWorld() const;
	void PlaceRocks() const;
	void PlaceMacro() const;
};
