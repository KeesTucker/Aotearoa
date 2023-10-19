#include "Environment/Actors/Current.h"

#include "Components/WindDirectionalSourceComponent.h"

ACurrent::ACurrent()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACurrent::BeginPlay()
{
	Super::BeginPlay();

	RandomStream.Initialize(FMath::Rand());
	
	WindComponent = FindComponentByClass<UWindDirectionalSourceComponent>();
	TargetSpeed = -WindComponent->Speed;

	CurrentChangeTime = RandomStream.FRandRange(0.6f * WaveDuration, 1.4 * WaveDuration);
}

void ACurrent::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	CurrentTime += DeltaTime;
	
	WindComponent->Speed = FMath::Lerp(PreviousSpeed, TargetSpeed, CurrentTime / CurrentChangeTime);
	
	if (CurrentTime > CurrentChangeTime)
	{
		CurrentTime = 0;
		CurrentChangeTime = RandomStream.FRandRange(0.6f * WaveDuration, 1.4 * WaveDuration);

		PreviousSpeed = TargetSpeed;
		TargetSpeed *= -1.0f;
	}
}

