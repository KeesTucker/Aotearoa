using UnrealBuildTool; 

public class ComputeShader: ModuleRules 

{
	public ComputeShader(ReadOnlyTargetRules target) : base(target) 

	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateIncludePaths.AddRange(new[] 
		{
			"ComputeShader/Private"
		});
		if (target.bBuildEditor)
			PrivateDependencyModuleNames.Add("TargetPlatform");
		PublicDependencyModuleNames.Add("Core");
		PublicDependencyModuleNames.Add("Engine");
		PublicDependencyModuleNames.Add("MaterialShaderQualitySettings");
		
		PrivateDependencyModuleNames.AddRange(new[]
		{
			"CoreUObject",
			"Renderer",
			"RenderCore",
			"RHI",
			"Projects"
		});

		if (!target.bBuildEditor) return;
		
		PrivateDependencyModuleNames.AddRange(
			new[] {
				"UnrealEd",
				"MaterialUtilities",
				"SlateCore",
				"Slate"
			}
		);

		CircularlyReferencedDependentModules.AddRange(
			new[] {
				"UnrealEd",
				"MaterialUtilities",
			}
		);
	}
}