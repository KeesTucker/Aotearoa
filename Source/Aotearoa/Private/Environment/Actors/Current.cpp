#include "Environment/Actors/Current.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

ACurrent::ACurrent(): WindMatParam(nullptr), WindComponent(nullptr), WindMatParamInstance(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACurrent::BeginPlay()
{
	Super::BeginPlay();
	
	WindComponent = FindComponentByClass<UWindDirectionalSourceComponent>();
	TargetSpeed = MaxWaveSpeed;

	CurrentChangeTime = WaveDuration;

	WindMatParamInstance = GetWorld()->GetParameterCollectionInstance(WindMatParam);

	WindMatParamInstance->SetVectorParameterValue("WaveDir", FLinearColor(GetActorForwardVector()));
	WindMatParamInstance->SetScalarParameterValue("MaxWaveSpeed", MaxWaveSpeed);
	WindMatParamInstance->SetScalarParameterValue("WaveDuration", WaveDuration);
}

void ACurrent::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	CurrentTime += DeltaTime;
	
	WindComponent->Speed = FMath::Lerp(PreviousSpeed, TargetSpeed, CurrentTime / CurrentChangeTime);
	
	if (CurrentTime > CurrentChangeTime)
	{
		CurrentTime = 0;
		CurrentChangeTime = WaveDuration;

		PreviousSpeed = TargetSpeed;
		TargetSpeed *= -1.0f;
	}
}

