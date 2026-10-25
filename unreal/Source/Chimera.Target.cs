using UnrealBuildTool;

public class ChimeraTarget : TargetRules
{
	public ChimeraTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Chimera");
	}
}
