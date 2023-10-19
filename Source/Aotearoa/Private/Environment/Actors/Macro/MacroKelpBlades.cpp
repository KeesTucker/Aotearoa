#include "Environment/Actors/Macro/MacroKelpBlades.h"

#include "Debug.h"
#include "EngineUtils.h"
#include "Components/SphereComponent.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "Engine/WindDirectionalSource.h"
#include "Kismet/GameplayStatics.h"

AMacroKelpBlades::AMacroKelpBlades()
{
	PrimaryActorTick.bCanEverTick = true;

	BladePhysicsComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SetRootComponent(BladePhysicsComponent);
	BladePhysicsComponent->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName); // Set collision profile as needed

	BladeVisualComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Blades"));
	BladeVisualComponent->SetupAttachment(BladePhysicsComponent);

	/*BladeSimComponent = CreateDefaultSubobject<UGeometryCacheComponent>(TEXT("BladesSim"));
	BladeSimComponent->SetupAttachment(BladePhysicsComponent);*/
}

void AMacroKelpBlades::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AMacroKelpBlades::BeginPlay()
{
	Super::BeginPlay();

	const TSubclassOf<AActor> WindActorClass = AWindDirectionalSource::StaticClass();
	
	for (const TActorIterator It(GetWorld(), WindActorClass); It;)
	{
		Wind = It->FindComponentByClass<UWindDirectionalSourceComponent>();
		break;
	}

	CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);

	BladeVisualComponent->bDisableClothSimulation = true;
	BladeVisualComponent->SetComponentTickEnabled(false);

	/*MID = UMaterialInstanceDynamic::Create(Mat, NULL);
	BladeVisualComponent->SetMaterial(0, MID);
	BladeVisualComponent->SetMaterial(1, MID);
	BladeVisualComponent->SetMaterial(2, MID);
	BladeVisualComponent->SetMaterial(3, MID);
	BladeVisualComponent->SetMaterial(4, MID);*/

	//SetActorScale3D(FVector(0, 0, 0));

	Disabled();
}

void AMacroKelpBlades::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(IsEnabled && !BladeVisualComponent->WasRecentlyRendered())
	{
		Disabled();
		IsEnabled = false;
	}
	else if(!IsEnabled && BladeVisualComponent->WasRecentlyRendered())
	{
		Enabled();
		IsEnabled = true;
	}
	
	if (IsEnabled)
	{
		/*if (TotalTime <= 10)
		{
			TotalTime += DeltaTime;
			MID->SetScalarParameterValue(TEXT("Growth"), TotalTime / 10.0f);
		}*/
		
		BladePhysicsComponent->AddForce(FVector(0, 0, 1) * FloatationFactor, NAME_None, true);
	
		if (Wind)
		{
			/*FWindData Data;
			Wind->GetWindParameters(GetOwner()->GetActorLocation(), Data, WindWeight);
		
			BladePhysicsComponent->AddForce(Data.Direction * Data.Speed, NAME_None, true);*/
	
			BladePhysicsComponent->AddForce(Wind->GetOwner()->GetActorForwardVector() * Wind->Speed * WindWeight, NAME_None, true);
		}

		if (!ClothSimDisabled)
		{
			const float Distance = FVector::Distance(CameraManager->GetCameraLocation(), GetActorLocation());
			if (BladeVisualComponent->bDisableClothSimulation && Distance <= SimDistance)
			{
				//BladeVisualComponent->ResumeClothingSimulation();
				BladeVisualComponent->bDisableClothSimulation = false;
				BladeVisualComponent->SetComponentTickEnabled(true);
				/*BladeSimComponent->Stop();
				BladeSimComponent->SetActive(false);
				BladeSimComponent->SetVisibility(false);*/
			}
			else if (!BladeVisualComponent->bDisableClothSimulation && Distance > SimDistance)
			{
				//BladeVisualComponent->SuspendClothingSimulation();
				BladeVisualComponent->bDisableClothSimulation = true;
				BladeVisualComponent->SetComponentTickEnabled(false);
				/*BladeSimComponent->Play();
				BladeSimComponent->SetActive(true);
				BladeSimComponent->SetVisibility(false);*/
			}
		}
	}
}

void AMacroKelpBlades::Enabled()
{
	SetActorEnableCollision(true);
	//BladeVisualComponent->SetActive(true);
	BladePhysicsComponent->SetActive(true);
	BladePhysicsComponent->SetSimulatePhysics(true);
	/*BladeVisualComponent->bDisableClothSimulation = false;
	BladeVisualComponent->SetComponentTickEnabled(true);*/
	//BladeSimComponent->Play();
	//BladeSimComponent->SetActive(true);
	//BladeSimComponent->SetVisibility(true);
	IsEnabled = true;
}

void AMacroKelpBlades::Disabled()
{
	SetActorEnableCollision(false);
	//BladeVisualComponent->SetActive(false);
	BladePhysicsComponent->SetActive(false);
	BladePhysicsComponent->SetSimulatePhysics(false);
	//BladeVisualComponent->bDisableClothSimulation = true;
	//BladeVisualComponent->SetComponentTickEnabled(false);
	//BladeSimComponent->Stop();
	/*BladeSimComponent->SetActive(false);
	BladeSimComponent->SetVisibility(false);*/
	IsEnabled = false;
}

