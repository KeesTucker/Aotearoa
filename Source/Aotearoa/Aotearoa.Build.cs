// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Aotearoa : ModuleRules
{
	public Aotearoa(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent", "RawMesh", "Landscape", "CableComponent", "GeometryCache"});

		PrivateDependencyModuleNames.AddRange(new string[] {"Utility", "ComputeShader", "Foliage"});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
