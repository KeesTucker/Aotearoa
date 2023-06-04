#pragma once
#include "MarchingCubesLookupTables.h"

class FMarchingCubesUtility
{
	static void AddVertex(const TArray<TArray<TArray<float>>>& Voxels, const FIntVector& VoxelPos, const FVector3f& WorldPos, const int* Edges,
								   const int EdgeIndex, TArray<FVector3f>& Vertices, TArray<uint32>& Triangles, TMap<FString,
								   int>& VertexMap, const float Isolevel, const float Scale, const int Resolution)
	{
		const int E00 = FMarchingCubesLookupTables::EdgeConnections[Edges[EdgeIndex]][0];
		const int E01 = FMarchingCubesLookupTables::EdgeConnections[Edges[EdgeIndex]][1];
		const FIntVector E00Index = VoxelPos + FMarchingCubesLookupTables::VertexOffsets[E00];
		const FIntVector E01Index = VoxelPos + FMarchingCubesLookupTables::VertexOffsets[E01];

		FString ID = FMarchingCubesLookupTables::VertexOffsets[E00].ToString();
		ID += FMarchingCubesLookupTables::VertexOffsets[E01].ToString();
		ID += VoxelPos.ToString();
		const FString UUID = LexToString(FMD5::HashAnsiString(*ID));

		int Index;
		if (VertexMap.Contains(UUID))
		{
			Index = VertexMap[UUID];
			Triangles.Add(Index);
		}
		else
		{
			FVector3f Vert = FMarchingCubesUtility::Interp(
					FVector3f(FMarchingCubesLookupTables::VertexOffsets[E00]),
					Voxels[E00Index.X][E00Index.Y][E00Index.Z],
					FVector3f(FMarchingCubesLookupTables::VertexOffsets[E01]),
					Voxels[E01Index.X][E01Index.Y][E01Index.Z],
					Isolevel
				) + WorldPos;
			Vert = Vert * Scale;
			Vertices.Add(Vert);
			Index = Vertices.Num() - 1;
			VertexMap.Add(UUID, Index);
			Triangles.Add(Index);
		}
	}
	
	static FVector3f Interp(const FVector3f& EdgeVertex1, const float ValueAtVertex1, const FVector3f& EdgeVertex2,
							const float ValueAtVertex2, const float IsoLevel)
	{
		return EdgeVertex1 + (IsoLevel - ValueAtVertex1) * (EdgeVertex2 - EdgeVertex1)  / (ValueAtVertex2 - ValueAtVertex1);
	}
	
public:
	struct FMeshOutput
	{
		TArray<FVector3f> Vertices;
		TArray<uint32> Triangles;
	};
	
	static FMeshOutput GenerateMesh(const int Resolution, const float Scale, const float Isolevel, const TArray<TArray<TArray<float>>>& Voxels)
	{
		TArray<FVector3f> Vertices = TArray<FVector3f>();
		TArray<uint32> Triangles = TArray<uint32>();
		TMap<FString, int> VertexMap = TMap<FString, int>();
	
		for (int x = 0; x < Resolution - 1; ++x)
		{
			for (int y = 0; y < Resolution - 1; ++y)
			{
				for (int z = 0; z < Resolution - 1; ++z)
				{
					FIntVector VoxelPos = FIntVector(x, y, z);
					int CubeIndex = 0;

					for (int i = 0; i < 8; ++i)
					{
						if (const FIntVector VertexPos = VoxelPos + FMarchingCubesLookupTables::VertexOffsets[i];
							Voxels[VertexPos.X][VertexPos.Y][VertexPos.Z] < Isolevel)
							CubeIndex |= static_cast<int>(std::pow(2, i));
					}
					
					const int* Edges = FMarchingCubesLookupTables::TriTable[CubeIndex];
					
					for (int i = 0; Edges[i] != -1; i += 3)
					{
						FVector3f ObjectOriginOffset = FVector3f(Resolution / 2, Resolution / 2, Resolution / 2);
						FVector3f WorldPos = FVector3f(VoxelPos) - ObjectOriginOffset;
						
						// First edge lies between vertex e00 and vertex e01
						AddVertex(Voxels, VoxelPos, WorldPos, Edges, i, Vertices, Triangles, VertexMap, Isolevel, Scale, Resolution);
	
						// Second edge lies between vertex e10 and vertex e11
						AddVertex(Voxels, VoxelPos, WorldPos, Edges, i + 1, Vertices, Triangles, VertexMap, Isolevel, Scale, Resolution);
    	    
						// Third edge lies between vertex e20 and vertex e21
						AddVertex(Voxels, VoxelPos, WorldPos, Edges, i + 2, Vertices, Triangles, VertexMap, Isolevel, Scale, Resolution);
					}
				}
			}
		}

		FMeshOutput Output;
		Output.Triangles = Triangles;
		Output.Vertices = Vertices;
		
		return Output;
	}
};
