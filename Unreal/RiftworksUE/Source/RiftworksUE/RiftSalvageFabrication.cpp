#include "RiftSalvageFabrication.h"

#include "RiftPersistence.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace RiftSalvagePrivate
{
    UStaticMesh* FindCube()
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
        return Mesh.Succeeded() ? Mesh.Object : nullptr;
    }

    UStaticMesh* FindCylinder()
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        return Mesh.Succeeded() ? Mesh.Object : nullptr;
    }

    FName RecoveryToolForTier(int32 Tier)
    {
        switch (Tier)
        {
            case 4: return TEXT("recovery_tool_t4");
            case 3: return TEXT("recovery_tool_t3");
            case 2: return TEXT("recovery_tool_t2");
            default: return NAME_None;
        }
    }
}

ARiftTieredSalvageActor::ARiftTieredSalvageActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

int32 ARiftTieredSalvageActor::GetPlayerRecoveryToolTier(const ARiftPlayerCharacter* Player)
{
    if (!Player)
    {
        return 1;
    }
    if (Player->GetComponentCount(TEXT("recovery_tool_t4")) > 0)
    {
        return 4;
    }
    if (Player->GetComponentCount(TEXT("recovery_tool_t3")) > 0)
    {
        return 3;
    }
    if (Player->GetComponentCount(TEXT("recovery_tool_t2")) > 0)
    {
        return 2;
    }
    return 1;
}

bool ARiftTieredSalvageActor::MeetsToolRequirement(const ARiftPlayerCharacter* Player) const
{
    return GetPlayerRecoveryToolTier(Player) >= FMath::Clamp(RecoveryTier, 1, 4);
}

FText ARiftTieredSalvageActor::GetInteractionText_Implementation() const
{
    const int32 Tier = FMath::Clamp(RecoveryTier, 1, 4);
    const FString Mechanical = Tier >= 3 && !bMechanicallyRecovered ? TEXT(" | WINCH/CRANE REQUIRED") : TEXT("");
    const FString Dismantle = bDismantleable ? TEXT(" | Crouch+E dismantle") : TEXT("");
    const FString Weight = bHeavy ? FString::Printf(TEXT(" | %.0f kg"), MassKg) : TEXT("");
    return FText::FromString(FString::Printf(
        TEXT("[E] Recover %s x%d | T%d%s%s%s"),
        *DisplayName.ToString(), Amount, Tier, *Weight, *Mechanical, *Dismantle));
}

void ARiftTieredSalvageActor::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    if (!Player)
    {
        return;
    }

    if (Player->bIsCrouched && bDismantleable)
    {
        Dismantle(Player);
        return;
    }

    if (!MeetsToolRequirement(Player))
    {
        const FName RequiredTool = RiftSalvagePrivate::RecoveryToolForTier(FMath::Clamp(RecoveryTier, 1, 4));
        Player->CurrentInteractionText = FText::FromString(FString::Printf(
            TEXT("Recovery blocked: Tier %d tools required (%s)"), RecoveryTier, *RequiredTool.ToString()));
        return;
    }

    if (RecoveryTier >= 3 && !bMechanicallyRecovered)
    {
        Player->CurrentInteractionText = FText::FromString(
            RecoveryTier >= 4 ? TEXT("Industrial recovery required: use a Tier 4 crane/rig") : TEXT("Mechanical recovery required: use a winch or vehicle rig"));
        return;
    }

    ARiftSalvageActor::Interact_Implementation(Player);
}

bool ARiftTieredSalvageActor::Dismantle(ARiftPlayerCharacter* Player)
{
    if (!Player || !bDismantleable || !MeetsToolRequirement(Player))
    {
        if (Player)
        {
            Player->CurrentInteractionText = FText::FromString(FString::Printf(TEXT("Cannot dismantle: Tier %d recovery tools required"), RecoveryTier));
        }
        return false;
    }

    if (DismantleScrapYield > 0)
    {
        Player->Scrap += DismantleScrapYield;
    }
    if (!DismantleComponentId.IsNone() && DismantleComponentAmount > 0)
    {
        Player->AddComponentItem(DismantleComponentId, DismantleComponentAmount);
    }
    else
    {
        Player->BP_OnInventoryChanged();
    }

    MarkRemovedFromPersistence(Player);
    Destroy();
    return true;
}

void ARiftTieredSalvageActor::MarkMechanicallyRecovered()
{
    bMechanicallyRecovered = true;
}

void ARiftTieredSalvageActor::MarkRemovedFromPersistence(ARiftPlayerCharacter* Player) const
{
    if (!Player || PersistentId.IsEmpty())
    {
        return;
    }
    if (UGameInstance* GameInstance = Player->GetGameInstance())
    {
        if (URiftPersistenceSubsystem* Persistence = GameInstance->GetSubsystem<URiftPersistenceSubsystem>())
        {
            Persistence->MarkSalvageRemoved(PersistentId);
        }
    }
}

ARiftRecoveryWinch::ARiftRecoveryWinch()
{
    PrimaryActorTick.bCanEverTick = true;

    Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RecoveryFrame"));
    SetRootComponent(Frame);
    Frame->SetStaticMesh(RiftSalvagePrivate::FindCube());
    Frame->SetRelativeScale3D(FVector(1.25f, 0.85f, 0.65f));
    Frame->SetCollisionProfileName(TEXT("BlockAll"));

    Spool = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RecoverySpool"));
    Spool->SetupAttachment(Frame);
    Spool->SetStaticMesh(RiftSalvagePrivate::FindCylinder());
    Spool->SetRelativeLocation(FVector(0.0f, 0.0f, 85.0f));
    Spool->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    Spool->SetRelativeScale3D(FVector(0.58f, 0.58f, 0.72f));
    Spool->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RecoveryStatus"));
    StatusLight->SetupAttachment(Frame);
    StatusLight->SetRelativeLocation(FVector(0.0f, -75.0f, 92.0f));
    StatusLight->IntensityUnits = ELightUnits::Lumens;
    StatusLight->SetIntensity(180.0f);
    StatusLight->SetAttenuationRadius(320.0f);
    StatusLight->SetLightColor(FLinearColor(0.82f, 0.42f, 0.08f));
    StatusLight->SetVolumetricScatteringIntensity(0.0f);
}

FText ARiftRecoveryWinch::GetInteractionText_Implementation() const
{
    if (AttachedSalvage)
    {
        return FText::FromString(FString::Printf(TEXT("[E] Release winch | pulling %s | capacity T%d"), *AttachedSalvage->DisplayName.ToString(), RecoveryCapacityTier));
    }
    return FText::FromString(FString::Printf(TEXT("[E] Attach recovery cable to aimed salvage | capacity T%d"), RecoveryCapacityTier));
}

void ARiftRecoveryWinch::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    if (!Player || !GetWorld() || !Player->FirstPersonCamera)
    {
        return;
    }

    if (AttachedSalvage)
    {
        ReleaseTarget();
        return;
    }

    const FVector Start = Player->FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + Player->FirstPersonCamera->GetForwardVector() * TargetRange;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftRecoveryWinchTrace), false, this);
    Params.AddIgnoredActor(Player);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        if (ARiftTieredSalvageActor* Target = Cast<ARiftTieredSalvageActor>(Hit.GetActor()))
        {
            if (!AttachTarget(Target))
            {
                Player->CurrentInteractionText = FText::FromString(TEXT("Recovery rig cannot move this tier"));
            }
            return;
        }
    }
    Player->CurrentInteractionText = FText::FromString(TEXT("Aim at a tiered salvage component, then operate the winch"));
}

bool ARiftRecoveryWinch::AttachTarget(ARiftTieredSalvageActor* Target)
{
    if (!Target || Target == AttachedSalvage || Target->RecoveryTier > RecoveryCapacityTier)
    {
        return false;
    }
    if (!Target->bHeavy || !Target->Mesh)
    {
        return false;
    }

    AttachedSalvage = Target;
    Target->SetCarriedState(false);
    Target->Mesh->SetSimulatePhysics(true);
    Target->Mesh->WakeAllRigidBodies();
    if (StatusLight)
    {
        StatusLight->SetLightColor(FLinearColor(0.10f, 0.82f, 0.28f));
        StatusLight->SetIntensity(320.0f);
    }
    return true;
}

void ARiftRecoveryWinch::ReleaseTarget()
{
    AttachedSalvage = nullptr;
    if (StatusLight)
    {
        StatusLight->SetLightColor(FLinearColor(0.82f, 0.42f, 0.08f));
        StatusLight->SetIntensity(180.0f);
    }
}

void ARiftRecoveryWinch::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!AttachedSalvage || !AttachedSalvage->Mesh)
    {
        return;
    }

    if (!IsValid(AttachedSalvage))
    {
        ReleaseTarget();
        return;
    }

    const FVector DockPoint = GetActorLocation() + GetActorForwardVector() * 150.0f + FVector(0.0f, 0.0f, 70.0f);
    const FVector Delta = DockPoint - AttachedSalvage->GetActorLocation();
    const float Distance = Delta.Size();
    if (Distance <= DockDistance)
    {
        AttachedSalvage->Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
        AttachedSalvage->Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        AttachedSalvage->MarkMechanicallyRecovered();
        ReleaseTarget();
        return;
    }

    if (AttachedSalvage->Mesh->IsSimulatingPhysics())
    {
        const float MassScale = FMath::Clamp(AttachedSalvage->MassKg / 35.0f, 0.75f, 5.0f);
        AttachedSalvage->Mesh->AddForce(Delta.GetSafeNormal() * PullForce * MassScale);
        AttachedSalvage->Mesh->SetLinearDamping(2.2f);
        Spool->AddLocalRotation(FRotator(0.0f, DeltaSeconds * 220.0f, 0.0f));
    }
}

ARiftFabricator::ARiftFabricator()
{
    PrimaryActorTick.bCanEverTick = false;

    Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FabricatorFrame"));
    SetRootComponent(Frame);
    Frame->SetStaticMesh(RiftSalvagePrivate::FindCube());
    Frame->SetRelativeScale3D(FVector(1.55f, 0.72f, 0.82f));
    Frame->SetCollisionProfileName(TEXT("BlockAll"));

    WorkSurface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FabricatorWorkSurface"));
    WorkSurface->SetupAttachment(Frame);
    WorkSurface->SetStaticMesh(RiftSalvagePrivate::FindCube());
    WorkSurface->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    WorkSurface->SetRelativeScale3D(FVector(1.08f, 0.82f, 0.12f));
    WorkSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FabricatorStatus"));
    StatusLight->SetupAttachment(Frame);
    StatusLight->SetRelativeLocation(FVector(0.0f, -76.0f, 95.0f));
    StatusLight->IntensityUnits = ELightUnits::Lumens;
    StatusLight->SetIntensity(220.0f);
    StatusLight->SetAttenuationRadius(360.0f);
    StatusLight->SetLightColor(FLinearColor(0.12f, 0.62f, 0.86f));
    StatusLight->SetVolumetricScatteringIntensity(0.0f);

    LastStatus = FText::FromString(TEXT("READY"));
}

FString ARiftFabricator::GetRecipeName() const
{
    switch (SelectedRecipe)
    {
        case ERiftFabricationRecipe::Ammunition: return TEXT("Rifle Ammunition x12");
        case ERiftFabricationRecipe::Cable: return TEXT("Utility Cable x2");
        case ERiftFabricationRecipe::Fasteners: return TEXT("Fasteners x8");
        case ERiftFabricationRecipe::StructuralPieces: return TEXT("Structural Plates x2");
        case ERiftFabricationRecipe::MedicalKit: return TEXT("Medical Kit x1");
        case ERiftFabricationRecipe::ReplacementPart: return TEXT("Replacement Part x1");
        default: return TEXT("Unknown Recipe");
    }
}

FString ARiftFabricator::GetRecipeCostText() const
{
    switch (SelectedRecipe)
    {
        case ERiftFabricationRecipe::Ammunition: return TEXT("3 scrap");
        case ERiftFabricationRecipe::Cable: return TEXT("2 scrap + 1 electronics");
        case ERiftFabricationRecipe::Fasteners: return TEXT("2 scrap");
        case ERiftFabricationRecipe::StructuralPieces: return TEXT("5 scrap");
        case ERiftFabricationRecipe::MedicalKit: return TEXT("1 scrap + 1 medical supplies");
        case ERiftFabricationRecipe::ReplacementPart: return TEXT("4 scrap + 1 electronics");
        default: return TEXT("?");
    }
}

FText ARiftFabricator::GetInteractionText_Implementation() const
{
    return FText::FromString(FString::Printf(
        TEXT("[E] Fabricate %s | %s | Crouch+E next recipe | %s"),
        *GetRecipeName(), *GetRecipeCostText(), *LastStatus.ToString()));
}

void ARiftFabricator::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    if (!Player)
    {
        return;
    }
    if (Player->bIsCrouched)
    {
        CycleRecipe(1);
        return;
    }
    Fabricate(Player);
}

void ARiftFabricator::CycleRecipe(int32 Direction)
{
    constexpr int32 RecipeCount = 6;
    int32 Value = static_cast<int32>(SelectedRecipe);
    Value = (Value + (Direction >= 0 ? 1 : -1) + RecipeCount) % RecipeCount;
    SelectedRecipe = static_cast<ERiftFabricationRecipe>(Value);
    LastStatus = FText::FromString(TEXT("RECIPE SELECTED"));
}

bool ARiftFabricator::Fabricate(ARiftPlayerCharacter* Player)
{
    if (!Player)
    {
        return false;
    }

    int32 ScrapCost = 0;
    FName ComponentCost = NAME_None;
    int32 ComponentCostAmount = 0;
    FName Output = NAME_None;
    int32 OutputAmount = 1;

    switch (SelectedRecipe)
    {
        case ERiftFabricationRecipe::Ammunition:
            ScrapCost = 3;
            Output = TEXT("ammo_rifle");
            OutputAmount = 12;
            break;
        case ERiftFabricationRecipe::Cable:
            ScrapCost = 2;
            ComponentCost = TEXT("electronics");
            ComponentCostAmount = 1;
            Output = TEXT("cable");
            OutputAmount = 2;
            break;
        case ERiftFabricationRecipe::Fasteners:
            ScrapCost = 2;
            Output = TEXT("bolts");
            OutputAmount = 8;
            break;
        case ERiftFabricationRecipe::StructuralPieces:
            ScrapCost = 5;
            Output = TEXT("structural_plate");
            OutputAmount = 2;
            break;
        case ERiftFabricationRecipe::MedicalKit:
            ScrapCost = 1;
            ComponentCost = TEXT("medical_supplies");
            ComponentCostAmount = 1;
            Output = TEXT("medical_kit");
            OutputAmount = 1;
            break;
        case ERiftFabricationRecipe::ReplacementPart:
            ScrapCost = 4;
            ComponentCost = TEXT("electronics");
            ComponentCostAmount = 1;
            Output = TEXT("replacement_part");
            OutputAmount = 1;
            break;
        default:
            return false;
    }

    if (Player->Scrap < ScrapCost || (!ComponentCost.IsNone() && Player->GetComponentCount(ComponentCost) < ComponentCostAmount))
    {
        LastStatus = FText::FromString(TEXT("INSUFFICIENT MATERIALS"));
        if (StatusLight)
        {
            StatusLight->SetLightColor(FLinearColor(0.88f, 0.12f, 0.06f));
        }
        return false;
    }

    Player->Scrap -= ScrapCost;
    if (!ComponentCost.IsNone() && ComponentCostAmount > 0)
    {
        Player->ConsumeComponentItem(ComponentCost, ComponentCostAmount);
    }
    Player->AddComponentItem(Output, OutputAmount);
    LastStatus = FText::FromString(TEXT("FABRICATION COMPLETE"));
    if (StatusLight)
    {
        StatusLight->SetLightColor(FLinearColor(0.10f, 0.82f, 0.28f));
    }
    return true;
}
