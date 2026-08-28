using UnrealBuildTool;

public class RiftworksUE : ModuleRules
{
    public RiftworksUE(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "AIModule",
            "NavigationSystem",
            "GameplayTasks",
            "PhysicsCore"
        });
    }
}
