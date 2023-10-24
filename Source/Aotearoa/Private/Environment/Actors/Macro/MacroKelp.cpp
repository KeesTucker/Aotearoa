#include "Environment/Actors/Macro/MacroKelp.h"

#include "Debug.h"
#include "Environment/Actors/Macro/MacroKelpBlades.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

AMacroKelp::AMacroKelp()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	SetRootComponent(BaseComponent);

	StemComponent = CreateDefaultSubobject<UCableComponent>(TEXT("Stem"));
	StemComponent->SetupAttachment(BaseComponent);
	StemComponent->CableLength = StemLength * 0.9f;

	StemConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("Stem Constraint"));
	StemConstraint->SetupAttachment(BaseComponent);
}

void AMacroKelp::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AMacroKelp::BeginPlay()
{
	Super::BeginPlay();

	const float RandStemLength = FMath::RandRange(StemLength * 0.7f, StemLength * 1.3f);
	
	BladeActor = GetWorld()->SpawnActor<AMacroKelpBlades>(Blades, GetActorLocation() + FVector(0, 0, RandStemLength), FRotator(0, FMath::RandRange(0.0f, 360.0f), 0));

	StemComponent->SetAttachEndTo(BladeActor, NAME_None);
	//StemComponent->CableLength = 1;
	StemComponent->CableLength = RandStemLength * 0.9f;

	StemConstraint->ConstraintActor1 = TObjectPtr<AActor>(this);
	StemConstraint->ConstraintActor2 = TObjectPtr<AActor>(BladeActor);
	StemConstraint->SetConstrainedComponents(
		BaseComponent, 
		NAME_None,
		Cast<UPrimitiveComponent>(BladeActor->GetRootComponent()),
		NAME_None);
	StemConstraint->SetLinearXLimit(LCM_Limited, 0);
	StemConstraint->SetLinearYLimit(LCM_Limited, 0);
	StemConstraint->SetLinearZLimit(LCM_Limited, 0);
	
	CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	
	SetActorEnableCollision(false);
	StemConstraint->SetActive(false);
	StemConstraint->SetComponentTickEnabled(false);
	Enabled = false;

	//SetActorScale3D(FVector(0, 0, 0));
	//StemComponent->CableLength = (TotalTime / 10.0f) * StemLeRandStemLengthngth * 0.9f + 1;
}

void AMacroKelp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/*if (Enabled)
	{
		if (TotalTime <= 10)
		{
			TotalTime += DeltaTime;
			SetActorScale3D(FVector(1, 1, 1) * FMath::Clamp(TotalTime / 10.0f, 0, 1));
			StemComponent->CableLength = (TotalTime / 10.0f) * RandStemLength * 0.9f + 1;
			StemConstraint->SetLinearXLimit(LCM_Limited, TotalTime / 10.0f * (RandStemLength - 10));
			StemConstraint->SetLinearYLimit(LCM_Limited, TotalTime / 10.0f * (RandStemLength - 10));
			StemConstraint->SetLinearZLimit(LCM_Limited, TotalTime / 10.0f * (RandStemLength - 10));
		}
	}*/
	
	if(Enabled && !BaseComponent->WasRecentlyRendered())
	{
		SetActorEnableCollision(false);
		StemConstraint->SetActive(false);
		StemConstraint->SetComponentTickEnabled(false);
		Enabled = false;
	}
	else if(!Enabled && BaseComponent->WasRecentlyRendered())
	{
		SetActorEnableCollision(true);
		StemConstraint->SetActive(true);
		StemConstraint->SetComponentTickEnabled(true);
		Enabled = true;
	}
}

