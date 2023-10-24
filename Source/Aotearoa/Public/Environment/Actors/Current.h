#pragma once

#include "CoreMinimal.h"
#include "Engine/WindDirectionalSource.h"
#include "Current.generated.h"

UCLASS()
class AOTEAROA_API ACurrent : public AWindDirectionalSource
{
	GENERATED_BODY()

public:
	ACurrent();

	virtual void Tick(float DeltaTime) override;

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
	UWindDirectionalSourceComponent* WindComponent;

	UPROPERTY()
	UMaterialParameterCollectionInstance* WindMatParamInstance;

	// WaveSum is sent to all materials that sway with the waves. It is a vector used to calculate WPO. 
	FVector WaveSum;
	float PreviousSpeed = 0;
	float TargetSpeed = 0;
	float CurrentChangeTime = 0;
	float CurrentTime = 0;
	bool bChanging = false;
};
