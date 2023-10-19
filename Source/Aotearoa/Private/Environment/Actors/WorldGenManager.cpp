#include "Environment/Actors/WorldGenManager.h"

#include "Debug.h"
#include "Landscape.h"
#include "Engine/StaticMeshActor.h"
#include "Environment/Actors/RockGenerator.h"
#include "Environment/Actors/Macro/MacroKelp.h"

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
	if (B_Generate)
	{
		B_Generate = false;
		Debug::Log(TEXT("Generating whole world is WIP, disabled currently"));
		//GenerateWorld();
	}
	if (B_PlaceRocks)
	{
		B_PlaceRocks = false;
		PlaceRocks();
	}
	if (B_PlaceMacro)
	{
		B_PlaceMacro = false;
		PlaceMacro();
	}
}

void AWorldGenManager::GenerateWorld() const
{
	const float Seed = 0.f;
	const int NumChunks = Size * ResolutionPerUnit / ResolutionPerChunk;
	const float ChunkSize = Size / NumChunks;
	
	for (int x = -NumChunks / 2; x < NumChunks / 2; ++x)
	{
		for (int y = -NumChunks / 2; y < NumChunks / 2; ++y)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Name = FName(*FString::Printf(TEXT("Chunk_X%d_Y%d"), x, y));
			TemplateGenerator->ResolutionPerUnit = ResolutionPerUnit;
			TemplateGenerator->Size = ChunkSize;
			TemplateGenerator->Seed = Seed;
			TemplateGenerator->ShapeModifier = EShapeModifier::EShapeModifier_Ground;
			SpawnParams.Template = Cast<AActor>(TemplateGenerator);

			ARockGenerator* Generator = GetWorld()->SpawnActor<ARockGenerator>(ARockGenerator::StaticClass(), SpawnParams);

			FVector SpawnLocation = FVector(ChunkSize * x, ChunkSize * y, 0);
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

void AWorldGenManager::PlaceRocks() const
{
	if (Landscape)
	{
		const FBox Bounds = Landscape->GetCompleteBounds();
		
		for (float x = Bounds.Min.X; x < Bounds.Max.X; x += 1 / SpawnDensity)
		{
			for (float y = Bounds.Min.Y; y < Bounds.Max.Y; y += 1 / SpawnDensity)
			{
				FVector Pos = FVector(
					x + FMath::RandRange(-MaxPosOffset, MaxPosOffset),
					y + FMath::RandRange(-MaxPosOffset, MaxPosOffset),
					0);

				TOptional<float> Height = Landscape->GetHeightAtLocation(Pos);
				
				if (!Height.IsSet())
				{
					continue;
				}
				
				Pos.Z = Height.GetValue();

				if (Pos.Z > SpawnHeight)
				{
					continue;
				}

				if (NoSpawnChance > (FMath::PerlinNoise3D(Pos * PerlinSize) + 1.0f) / 2.0f)
				{
					continue;
				}
				
				Pos.Z += FMath::RandRange(-MaxHeightOffset, 0.0f);

				FRotator Rot = FRotator(
					FMath::RandRange(0.0f, 360.0f),
					FMath::RandRange(0.0f, 360.0f),
					FMath::RandRange(0.0f, 360.0f));

				const AStaticMeshActor* Rock = GetWorld()->SpawnActor<AStaticMeshActor>(Pos, Rot);
				Rock->GetStaticMeshComponent()->SetStaticMesh(Rocks[FMath::RandRange(0, Rocks.Num() - 1)]);
				Rock->GetStaticMeshComponent()->SetWorldScale3D(FVector(1, 1, 1) * FMath::RandRange(MinScale, MaxScale));
			}
		}
	}
}

void AWorldGenManager::PlaceMacro() const
{
	if (Landscape)
	{
		const FBox Bounds = Landscape->GetCompleteBounds();
		
		for (float x = Bounds.Min.X; x < Bounds.Max.X; x += 1 / SpawnDensity)
		{
			for (float y = Bounds.Min.Y; y < Bounds.Max.Y; y += 1 / SpawnDensity)
			{
				FVector Pos = FVector(
					x + FMath::RandRange(-MaxPosOffset, MaxPosOffset),
					y + FMath::RandRange(-MaxPosOffset, MaxPosOffset),
					0);

				FVector StartLocation = Pos;
				FVector EndLocation = StartLocation - FVector(0, 0, 1000);

				FHitResult HitResult;
				FCollisionQueryParams CollisionParams;
				CollisionParams.AddIgnoredActor(Landscape);

				FRotator Rot;
				
				if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_WorldStatic, CollisionParams))
				{
					if (HitResult.GetActor()->IsA(MacroBP))
					{
						continue;
					}
					FVector HitNormal = HitResult.Normal;
					HitNormal.Normalize();
					Pos = HitResult.Location - HitNormal * 3.0f;
					Rot = FRotationMatrix::MakeFromZ(HitNormal).Rotator();
				}
				else
				{
					continue;
				}

				if (Pos.Z > SpawnHeight)
				{
					continue;
				}

				if (NoSpawnChance > (FMath::PerlinNoise3D(Pos * PerlinSize) + 1.0f) / 2.0f)
				{
					continue;
				}

				GetWorld()->SpawnActor<AMacroKelp>(MacroBP, Pos, Rot);
			}
		}
	}
}

