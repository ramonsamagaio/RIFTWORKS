#include "RiftGameplayActors.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectGlobals.h"

static UMaterialInterface* RiftLoadVisualMaterial(const TCHAR* AssetPath)
{
    return Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, AssetPath));
}

void ARiftSalvageActor::BeginPlay()
{
    Super::BeginPlay();
    SetCarriedState(false);

    if (Mesh)
    {
        if (UMaterialInterface* Material = RiftLoadVisualMaterial(TEXT("/Game/Riftworks/Materials/World/M_Salvage_Utility.M_Salvage_Utility")))
        {
            Mesh->SetMaterial(0, Material);
        }
    }
}

void ARiftAssemblyPart::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ConfigurePart();

    if (!PhysicsMesh)
    {
        return;
    }

    // Engine BasicShapes Cylinder is Z-axis aligned. FAS defines the vehicle
    // forward axis as +X and applies motor torque around ActorRightVector (+Y),
    // so wheel/motor-wheel axles must also be local +Y. Roll 90 maps cylinder Z
    // onto local Y. This normalization intentionally runs after ConfigurePart()
    // so legacy Blueprint defaults cannot restore the old X-axis wheel pose.
    if (PartType == ERiftAssemblyPartType::Wheel || PartType == ERiftAssemblyPartType::MotorWheel)
    {
        PhysicsMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
    }

    const TCHAR* MaterialPath = TEXT("/Game/Riftworks/Materials/World/M_Assembly_Steel.M_Assembly_Steel");
    if (PartType == ERiftAssemblyPartType::MotorWheel)
    {
        MaterialPath = TEXT("/Game/Riftworks/Materials/World/M_Assembly_Motor.M_Assembly_Motor");
    }
    else if (PartType == ERiftAssemblyPartType::Wheel)
    {
        MaterialPath = TEXT("/Game/Riftworks/Materials/World/M_Rubber_Dark.M_Rubber_Dark");
    }

    if (UMaterialInterface* Material = RiftLoadVisualMaterial(MaterialPath))
    {
        PhysicsMesh->SetMaterial(0, Material);
    }
}
