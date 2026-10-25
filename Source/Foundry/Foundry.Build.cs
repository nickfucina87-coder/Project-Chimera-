using UnrealBuildTool;

public class Foundry : ModuleRules
{
	public Foundry(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore", "ProceduralMeshComponent",
			"Niagara", "HTTP", "Json", "JsonUtilities"
		});
	}
}
