#include "Environment/Actors/Current.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

ACurrent::ACurrent(): WindMatParam(nullptr), WindMatParamInstance(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACurrent::BeginPlay()
{
	Super::BeginPlay();

	WindMatParamInstance = GetWorld()->GetParameterCollectionInstance(WindMatParam);

	WindMatParamInstance->SetVectorParameterValue("WaveDir", FLinearColor(GetActorForwardVector()));
	WindMatParamInstance->SetScalarParameterValue("MaxWaveSpeed", MaxWaveSpeed);
	WindMatParamInstance->SetScalarParameterValue("WaveDuration", WaveDuration);
}

