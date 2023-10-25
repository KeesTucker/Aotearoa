#pragma once

#include "CoreMinimal.h"
#include "Current.generated.h"

UCLASS()
class AOTEAROA_API ACurrent : public AActor
{
	GENERATED_BODY()

public:
	ACurrent();

protected:
	UPROPERTY(EditAnywhere)
	float WaveDuration = 5;
	UPROPERTY(EditAnywhere)
	float MaxWaveSpeed = 5;
	UPROPERTY(EditAnywhere)
	UMaterialParameterCollection* WindMatParam;
	
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UMaterialParameterCollectionInstance* WindMatParamInstance;
};
