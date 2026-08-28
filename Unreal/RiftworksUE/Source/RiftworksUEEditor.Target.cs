using UnrealBuildTool;
using System.Collections.Generic;

public class RiftworksUEEditorTarget : TargetRules
{
    public RiftworksUEEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("RiftworksUE");
    }
}
