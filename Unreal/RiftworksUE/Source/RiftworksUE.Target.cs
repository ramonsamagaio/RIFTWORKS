using UnrealBuildTool;
using System.Collections.Generic;

public class RiftworksUETarget : TargetRules
{
    public RiftworksUETarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("RiftworksUE");
    }
}
