#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeometryCacheComponent.h"
#include "MacroKelpBlades.generated.h"

class AWindDirectionalSource;
class USphereComponent;

UCLASS()
class AOTEAROA_API AMacroKelpBlades : public AActor
{
	GENERATED_BODY()

public:
	AMacroKelpBlades();

	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere)
	USkeletalMeshComponent* BladeVisualComponent;
	/*UPROPERTY(EditAnywhere)
	UGeometryCacheComponent* BladeSimComponent;*/
	UPROPERTY(EditAnywhere)
	USphereComponent* BladePhysicsComponent;
	UPROPERTY(EditAnywhere)
	UMaterialInstance* Mat;
	UPROPERTY(EditAnywhere)
	float WindWeight = 1;
	UPROPERTY(EditAnywhere)
	float FloatationFactor = 10;
	UPROPERTY(EditAnywhere)
	float SimDistance = 400;
	UPROPERTY(EditAnywhere)
	bool ClothSimDisabled = false;
	
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

private:
	UPROPERTY()
	UWindDirectionalSourceComponent* Wind;
	UPROPERTY()
	APlayerCameraManager* CameraManager;
	UPROPERTY()
	bool IsEnabled = false;
	//UPROPERTY()
	//float TotalTime = 0;
	//UPROPERTY()
	//UMaterialInstanceDynamic* MID;

	void Enabled();
	void Disabled();
};
