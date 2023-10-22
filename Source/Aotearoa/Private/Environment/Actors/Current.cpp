#include "Environment/Actors/Current.h"

#include "FoliageInstancedStaticMeshComponent.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "Math/UnitConversion.h"

ACurrent::ACurrent(): Macro(nullptr), MacroMI(nullptr), WindComponent(nullptr), MacroDMI(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACurrent::BeginPlay()
{
	Super::BeginPlay();

	RandomStream.Initialize(FMath::Rand());
	
	WindComponent = FindComponentByClass<UWindDirectionalSourceComponent>();
	TargetSpeed = MaxWaveSpeed;

	CurrentChangeTime = RandomStream.FRandRange(0.6f * WaveDuration, 1.4 * WaveDuration);

	MacroDMI = UMaterialInstanceDynamic::Create(MacroMI, nullptr);

	TArray<UFoliageInstancedStaticMeshComponent*> MacroInstances;
	Macro->GetComponents(MacroInstances);

	for (const auto MacroInstance : MacroInstances)
	{
		MacroInstance->SetMaterial(0, MacroDMI);
		MacroInstance->SetMaterial(1, MacroDMI);
	}
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
	
	MacroWindValue = GetActorForwardVector() * WindComponent->Speed * WindVertexOffsetMultiplier * DeltaTime + MacroWindValue;
	
	MacroDMI->SetVectorParameterValue("Wind", MacroWindValue - GetActorForwardVector() * WindVertexOffset);
}

