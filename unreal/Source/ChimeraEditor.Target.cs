using UnrealBuildTool;

public class ChimeraEditorTarget : TargetRules
{
	public ChimeraEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Chimera");
	}
}
