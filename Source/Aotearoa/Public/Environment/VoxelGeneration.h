#pragma once
#include "FastNoiseLite.h"
#include "Dispatch/VoxelDensityComputeShader.h"
#include "VoxelGeneration.generated.h"

UENUM(BlueprintType)
enum class EShapeModifier : uint8
{
	EShapeModifier_Sphere = 0,
	EShapeModifier_Ground = 1
};

UENUM(BlueprintType)
enum class ENoiseTypeWrapper :uint8
{
	NoiseType_OpenSimplex2 = 0,
	NoiseType_OpenSimplex2S = 1,
	NoiseType_Cellular = 2,
	NoiseType_Perlin = 3,
	NoiseType_ValueCubic = 4,
	NoiseType_Value = 5
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

	FComputeNoiseLayer ToComputeNoiseLayer() const
	{
		FComputeNoiseLayer ComputeNoiseLayer;
		ComputeNoiseLayer.NoiseType = static_cast<int>(NoiseType);
		ComputeNoiseLayer.Scale = Scale;
		ComputeNoiseLayer.Strength = Strength;
		return ComputeNoiseLayer;
	}
};

class FVoxelGeneration
{
public:
	static TArray<TArray<TArray<float>>> GenerateVoxelsWithNoise(const int Resolution, const float Seed,
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
						ShapeDensity = static_cast<float>(Resolution - z) / Resolution;
						break;
					default:
						ShapeDensity = 0;
					}
					

					float NoiseDensity = 0;
					for (const auto [NoiseType, Scale, Strength] : NoiseLayers)
					{
						Noise.SetNoiseType(static_cast<FastNoiseLite::NoiseType>(NoiseType));
						NoiseDensity += (Noise.GetNoise(
							x * Scale / Resolution + Seed,
							y * Scale / Resolution + Seed,
							z * Scale / Resolution + Seed)
							+ 0.5f) * Strength;
					}
					Voxels[x][y][z] = ShapeDensity + NoiseDensity;
				}
			}
		}

		return Voxels;
	}
};
