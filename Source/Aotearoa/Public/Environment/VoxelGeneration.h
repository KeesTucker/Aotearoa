#pragma once
#include "FastNoiseLite.h"
#include "VoxelGeneration.generated.h"

UENUM(BlueprintType)
enum class EShapeModifier : uint8
{
	EShapeModifier_Sphere,
	EShapeModifier_Ground
};

UENUM(BlueprintType)
enum class ENoiseTypeWrapper :uint8
{
	NoiseType_OpenSimplex2,
	NoiseType_OpenSimplex2S,
	NoiseType_Cellular,
	NoiseType_Perlin,
	NoiseType_ValueCubic,
	NoiseType_Value
};

USTRUCT(BlueprintType)
struct FNoiseLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	ENoiseTypeWrapper NoiseType = ENoiseTypeWrapper::NoiseType_Perlin;
	UPROPERTY(EditAnywhere)
	float Scale = 100.f;
	UPROPERTY(EditAnywhere)
	float Strength = 0.f;
};

class FVoxelGeneration
{
public:
	static TArray<TArray<TArray<float>>> GenerateVoxelsWithNoise(const int Resolution,
		const EShapeModifier ShapeModifier, TArray<FNoiseLayer> NoiseLayers)
	{
		FastNoiseLite Noise = FastNoiseLite();
		
		TArray<TArray<TArray<float>>> Voxels = TArray<TArray<TArray<float>>>();
		Voxels.SetNum(Resolution);
		for (int x = 0; x < Resolution; ++x)
		{
			Voxels[x].SetNum(Resolution);
			for (int y = 0; y < Resolution; ++y)
			{
				Voxels[x][y].SetNum(Resolution);
				for (int z = 0; z < Resolution; ++z)
				{
					float ShapeDensity;
					switch (ShapeModifier)
					{
					case EShapeModifier::EShapeModifier_Sphere:
						{
							// Distance from center
							const float Distance = FVector3f::Dist(
								FVector3f(x, y, z),
								FVector3f(
								Resolution / 2,
								Resolution / 2,
								Resolution / 2));
							// Normalize
							ShapeDensity = 1.f - Distance / (Resolution / 2);
							// Clamp
							ShapeDensity = FMath::Clamp(ShapeDensity, 0.f, 1.f);
							break;
						}
					case EShapeModifier::EShapeModifier_Ground:
						ShapeDensity = y - Resolution / 2;
						break;
					default:
						ShapeDensity = 0;
					}
					

					float NoiseDensity = 0;
					for (const auto [NoiseType, Scale, Strength] : NoiseLayers)
					{
						Noise.SetNoiseType(static_cast<FastNoiseLite::NoiseType>(NoiseType));
						NoiseDensity += (Noise.GetNoise(
							x * Scale / Resolution,
							y * Scale / Resolution,
							z * Scale / Resolution)
							+ 0.5f) * Strength;
					}
					Voxels[x][y][z] = ShapeDensity + NoiseDensity;
				}
			}
		}

		return Voxels;
	}
};
