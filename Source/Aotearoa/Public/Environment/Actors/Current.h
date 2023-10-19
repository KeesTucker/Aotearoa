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
	
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UWindDirectionalSourceComponent* WindComponent;
	
	FRandomStream RandomStream;
	float PreviousSpeed = 0;
	float TargetSpeed = 0;
	float CurrentChangeTime = 0;
	float CurrentTime = 0;
	bool bChanging = false;
};
