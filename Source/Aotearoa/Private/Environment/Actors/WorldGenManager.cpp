#include "Environment/Actors/WorldGenManager.h"

#include "Environment/Actors/RockGenerator.h"

AWorldGenManager::AWorldGenManager()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AWorldGenManager::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWorldGenManager::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	if (Generate)
	{
		Generate = false;
		GenerateWorld();
	}
}

void AWorldGenManager::GenerateWorld() const
{
	const float Seed = FMath::RandRange(-69696969.f, 69696969.f);
	const int NumChunks = Size * ResolutionPerUnit / ResolutionPerChunk;
	const float ChunkSize = Size / NumChunks;
	
	for (int x = 0; x < NumChunks; ++x)
	{
		for (int y = 0; y < NumChunks; ++y)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Name = FName(*FString::Printf(TEXT("Chunk_X%d_Y%d"), x, y));
			TemplateGenerator->ResolutionPerUnit = ResolutionPerUnit;
			TemplateGenerator->Size = ChunkSize;
			TemplateGenerator->Seed = Seed;
			TemplateGenerator->ShapeModifier = EShapeModifier::EShapeModifier_Ground;
			SpawnParams.Template = Cast<AActor>(TemplateGenerator);

			ARockGenerator* Generator = GetWorld()->SpawnActor<ARockGenerator>(ARockGenerator::StaticClass(), SpawnParams);

			FVector SpawnLocation = FVector(ChunkSize * (x - NumChunks / 2.f), ChunkSize * (y - NumChunks / 2.f), 0);
			Generator->SetActorLocation(SpawnLocation);
			Generator->SetActorLabel(FString::Printf(TEXT("Ground_X%d_Y%d"), x, y));

			FString Name = TemplateGenerator->Name;
			Generator->Name = FString::Printf(TEXT("%sGround_X%d_Y%d"), *Name, x, y);
			Generator->Offset = FVector3f(SpawnLocation);

			TFuture<void> CurrentGenerator;
			Async(EAsyncExecution::TaskGraphMainThread, [Generator]() mutable {
				Generator->GenerateAndUpdateMesh();
			});
			CurrentGenerator.Wait();
		}
	}
}

