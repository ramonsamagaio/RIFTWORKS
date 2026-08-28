using UnrealBuildTool;
using System.Collections.Generic;

public class RiftworksUEEditorTarget : TargetRules
{
    public RiftworksUEEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("RiftworksUE");
    }
}
