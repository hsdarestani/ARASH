using UnrealBuildTool;
using System.Collections.Generic;

public class ARASHEditorTarget : TargetRules
{
    public ARASHEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("ARASH");
    }
}
