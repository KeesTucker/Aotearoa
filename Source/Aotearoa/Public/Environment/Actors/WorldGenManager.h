#pragma once

#include "CoreMinimal.h"
#include "RockGenerator.h"
#include "GameFramework/Actor.h"
#include "WorldGenManager.generated.h"

UCLASS()
class AOTEAROA_API AWorldGenManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AWorldGenManager();

	UPROPERTY(EditAnywhere)
	bool Generate = false;

	UPROPERTY(EditAnywhere)
	float Size = 1024.f;
	UPROPERTY(EditAnywhere)
	int ResolutionPerChunk = 256;
	UPROPERTY(EditAnywhere)
	float ResolutionPerUnit = 1.f;
	UPROPERTY(EditAnywhere)
	ARockGenerator* TemplateGenerator;

protected:
	virtual void BeginPlay() override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;

private:
	void GenerateWorld() const;
};
