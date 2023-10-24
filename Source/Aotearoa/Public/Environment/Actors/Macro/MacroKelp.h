#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CableComponent.h"
#include "MacroKelp.generated.h"

class AMacroKelpBlades;
class UPhysicsConstraintComponent;
class USphereComponent;

UCLASS()
class AOTEAROA_API AMacroKelp : public AActor
{
	GENERATED_BODY()

public:
	AMacroKelp();

	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* BaseComponent; 
	UPROPERTY(EditAnywhere)
	UCableComponent* StemComponent;
	UPROPERTY(EditAnywhere)
	UPhysicsConstraintComponent* StemConstraint;

	UPROPERTY(EditAnywhere)
	UClass* Blades;

	UPROPERTY(EditAnywhere)
	float StemLength = 25;
	
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

private:
	UPROPERTY()
	AMacroKelpBlades* BladeActor;
	UPROPERTY()
	APlayerCameraManager* CameraManager;
	UPROPERTY()
	bool Enabled = true;
	//UPROPERTY()
	//float TotalTime = 0;
};
