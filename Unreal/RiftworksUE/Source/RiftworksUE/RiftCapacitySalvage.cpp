#include "RiftCapacitySalvage.h"

#include "RiftInventoryRules.h"
#include "RiftPlayerCharacter.h"

bool ARiftCapacitySalvageActor::CanAcceptDismantleYield(ARiftPlayerCharacter* Player, FText& OutReason) const
{
    if (!Player)
    {
        OutReason = FText::FromString(TEXT("No player inventory"));
        return false;
    }

    const float CurrentMass = URiftInventoryRules::GetInventoryMassKg(Player);
    const float CurrentVolume = URiftInventoryRules::GetInventoryVolumeL(Player);
    float AddedMass = 0.0f;
    float AddedVolume = 0.0f;

    if (DismantleScrapYield > 0)
    {
        const FRiftItemPhysicalProfile ScrapProfile = URiftInventoryRules::GetItemProfile(TEXT("scrap"));
        AddedMass += ScrapProfile.UnitMassKg * DismantleScrapYield;
        AddedVolume += ScrapProfile.UnitVolumeL * DismantleScrapYield;
    }

    if (!DismantleComponentId.IsNone() && DismantleComponentAmount > 0)
    {
        const FRiftItemPhysicalProfile ComponentProfile = URiftInventoryRules::GetItemProfile(DismantleComponentId);
        AddedMass += ComponentProfile.UnitMassKg * DismantleComponentAmount;
        AddedVolume += ComponentProfile.UnitVolumeL * DismantleComponentAmount;
    }

    const float NewMass = CurrentMass + AddedMass;
    const float NewVolume = CurrentVolume + AddedVolume;
    if (NewMass > URiftInventoryRules::GetMaximumMassKg() + KINDA_SMALL_NUMBER)
    {
        OutReason = FText::FromString(FString::Printf(
            TEXT("Cannot dismantle here: %.1f / %.1f kg"),
            NewMass,
            URiftInventoryRules::GetMaximumMassKg()));
        return false;
    }
    if (NewVolume > URiftInventoryRules::GetMaximumVolumeL() + KINDA_SMALL_NUMBER)
    {
        OutReason = FText::FromString(FString::Printf(
            TEXT("Cannot dismantle here: %.1f / %.1f L"),
            NewVolume,
            URiftInventoryRules::GetMaximumVolumeL()));
        return false;
    }

    OutReason = FText::GetEmpty();
    return true;
}

void ARiftCapacitySalvageActor::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    if (!Player)
    {
        return;
    }

    if (Player->bIsCrouched && bDismantleable)
    {
        FText Reason;
        if (!CanAcceptDismantleYield(Player, Reason))
        {
            Player->CurrentInteractionText = Reason;
            return;
        }
        ARiftTieredSalvageActor::Interact_Implementation(Player);
        return;
    }

    if (!bHeavy)
    {
        FText Reason;
        if (!URiftInventoryRules::CanAcceptItem(Player, ItemId, Amount, Reason))
        {
            Player->CurrentInteractionText = Reason;
            return;
        }
    }

    ARiftTieredSalvageActor::Interact_Implementation(Player);
}
