#include "RiftProductionPlayer.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"

AActor* ARiftProductionPlayerCharacter::TraceEngineeringActor(FHitResult* OutHit) const
{
    if (!FirstPersonCamera || !GetWorld())
    {
        return nullptr;
    }

    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * EngineeringTraceDistance;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftLogicTrace), false, this);
    if (BuildPreview)
    {
        Params.AddIgnoredActor(BuildPreview);
    }
    if (CarriedSalvage)
    {
        Params.AddIgnoredActor(CarriedSalvage);
    }

    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        return nullptr;
    }
    if (OutHit)
    {
        *OutHit = Hit;
    }
    return Hit.GetActor();
}

FString ARiftProductionPlayerCharacter::UtilityModeName() const
{
    const UEnum* Enum = StaticEnum<ERiftUtilityBuildMode>();
    return Enum ? Enum->GetNameStringByValue(static_cast<int64>(SelectedUtilityBuildMode)) : TEXT("Utility");
}

TSubclassOf<ARiftLogicNode> ARiftProductionPlayerCharacter::GetSelectedUtilityClass() const
{
    switch (SelectedUtilityBuildMode)
    {
        case ERiftUtilityBuildMode::Button: return LogicButtonClass;
        case ERiftUtilityBuildMode::ProximitySensor: return LogicSensorClass;
        case ERiftUtilityBuildMode::Timer: return LogicTimerClass;
        default: return nullptr;
    }
}

FString ARiftProductionPlayerCharacter::GetUtilityBuildCostText() const
{
    switch (SelectedUtilityBuildMode)
    {
        case ERiftUtilityBuildMode::Button: return TEXT("1 scrap + 1 electronics");
        case ERiftUtilityBuildMode::ProximitySensor: return TEXT("2 scrap + 1 electronics");
        case ERiftUtilityBuildMode::Timer: return TEXT("2 scrap + 1 electronics");
        default: return TEXT("materials");
    }
}

bool ARiftProductionPlayerCharacter::CanAffordUtility() const
{
    const int32 ScrapCost = SelectedUtilityBuildMode == ERiftUtilityBuildMode::Button ? 1 : 2;
    return Scrap >= ScrapCost && GetComponentCount(TEXT("electronics")) >= 1;
}

bool ARiftProductionPlayerCharacter::ConsumeUtilityCost()
{
    if (!CanAffordUtility())
    {
        return false;
    }

    Scrap -= SelectedUtilityBuildMode == ERiftUtilityBuildMode::Button ? 1 : 2;
    ConsumeComponentItem(TEXT("electronics"), 1);
    BP_OnInventoryChanged();
    return true;
}

void ARiftProductionPlayerCharacter::CycleUtilityBuildMode(int32 Direction)
{
    constexpr int32 ModeCount = 3;
    int32 Value = static_cast<int32>(SelectedUtilityBuildMode);
    Value = (Value + (Direction >= 0 ? 1 : -1) + ModeCount) % ModeCount;
    SelectedUtilityBuildMode = static_cast<ERiftUtilityBuildMode>(Value);
    CurrentInteractionText = FText::FromString(FString::Printf(
        TEXT("UTILITY %s | MMB place | cost %s"),
        *UtilityModeName(),
        *GetUtilityBuildCostText()));
}

void ARiftProductionPlayerCharacter::UtilityBuildNextPressed()
{
    if (bBuildMode)
    {
        return;
    }
    CycleUtilityBuildMode(1);
}

void ARiftProductionPlayerCharacter::UtilityBuildPlacePressed()
{
    if (bBuildMode || !GetWorld() || !FirstPersonCamera)
    {
        return;
    }

    TSubclassOf<ARiftLogicNode> UtilityClass = GetSelectedUtilityClass();
    if (!UtilityClass)
    {
        CurrentInteractionText = FText::FromString(TEXT("Logic Blueprint class is not configured yet"));
        return;
    }
    if (!CanAffordUtility())
    {
        CurrentInteractionText = FText::FromString(FString::Printf(
            TEXT("NEED MATERIALS | %s costs %s"),
            *UtilityModeName(),
            *GetUtilityBuildCostText()));
        return;
    }

    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * BuildDistance;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftUtilityPlacement), false, this);
    if (CarriedSalvage)
    {
        Params.AddIgnoredActor(CarriedSalvage);
    }

    FVector Location = End;
    FVector Normal = FVector::UpVector;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        Location = Hit.ImpactPoint + Hit.ImpactNormal * 18.0f;
        Normal = Hit.ImpactNormal;
    }
    Location.X = FMath::GridSnap(Location.X, BuildGridSize);
    Location.Y = FMath::GridSnap(Location.Y, BuildGridSize);
    Location.Z = FMath::GridSnap(Location.Z, BuildGridSize);

    FRotator Rotation = Normal.Rotation();
    Rotation.Pitch -= 90.0f;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    ARiftLogicNode* Node = GetWorld()->SpawnActor<ARiftLogicNode>(UtilityClass, Location, Rotation, SpawnParams);
    if (!Node)
    {
        CurrentInteractionText = FText::FromString(TEXT("Could not place logic utility"));
        return;
    }

    if (!ConsumeUtilityCost())
    {
        Node->Destroy();
        return;
    }

    CurrentInteractionText = FText::FromString(FString::Printf(
        TEXT("%s placed | Y link source to receiver"), *UtilityModeName()));
}

void ARiftProductionPlayerCharacter::LogicConnectPressed()
{
    if (bBuildMode)
    {
        return;
    }

    AActor* AimedActor = TraceEngineeringActor();
    if (!LogicSelectionSource || !IsValid(LogicSelectionSource))
    {
        ARiftLogicNode* Source = Cast<ARiftLogicNode>(AimedActor);
        if (!Source)
        {
            CurrentInteractionText = FText::FromString(TEXT("Y LINK: aim at a Button, Sensor or Timer first"));
            return;
        }
        LogicSelectionSource = Source;
        CurrentInteractionText = FText::FromString(FString::Printf(
            TEXT("LOGIC SOURCE %s | aim at motor, joint, light, field or device and press Y"),
            *Source->GetName()));
        return;
    }

    if (!AimedActor || AimedActor == LogicSelectionSource)
    {
        LogicSelectionSource = nullptr;
        CurrentInteractionText = FText::FromString(TEXT("Logic link selection cancelled"));
        return;
    }

    LogicSelectionSource->ConnectReceiver(AimedActor);
    BP_OnLogicLinkCreated(LogicSelectionSource, AimedActor);
    CurrentInteractionText = FText::FromString(FString::Printf(
        TEXT("LOGIC LINK: %s -> %s"),
        *LogicSelectionSource->GetName(),
        *AimedActor->GetName()));
    LogicSelectionSource = nullptr;
}
