#include "RiftInventoryRules.h"

#include "RiftPlayerCharacter.h"

namespace RiftInventoryPrivate
{
    constexpr float MaxMassKg = 34.0f;
    constexpr float MaxVolumeL = 52.0f;

    FRiftItemPhysicalProfile Profile(float MassKg, float VolumeL)
    {
        FRiftItemPhysicalProfile Result;
        Result.UnitMassKg = MassKg;
        Result.UnitVolumeL = VolumeL;
        return Result;
    }
}

FRiftItemPhysicalProfile URiftInventoryRules::ResolveProfile(FName ItemId)
{
    if (ItemId == TEXT("scrap")) return RiftInventoryPrivate::Profile(0.12f, 0.10f);
    if (ItemId == TEXT("electronics")) return RiftInventoryPrivate::Profile(0.45f, 0.65f);
    if (ItemId == TEXT("battery")) return RiftInventoryPrivate::Profile(0.85f, 0.70f);
    if (ItemId == TEXT("cable")) return RiftInventoryPrivate::Profile(0.90f, 1.15f);
    if (ItemId == TEXT("fuel")) return RiftInventoryPrivate::Profile(1.80f, 2.40f);
    if (ItemId == TEXT("copper_coil")) return RiftInventoryPrivate::Profile(2.50f, 2.20f);
    if (ItemId == TEXT("ammo")) return RiftInventoryPrivate::Profile(0.32f, 0.22f);
    if (ItemId == TEXT("fasteners")) return RiftInventoryPrivate::Profile(0.08f, 0.06f);
    if (ItemId == TEXT("structural_plate")) return RiftInventoryPrivate::Profile(1.65f, 1.40f);
    if (ItemId == TEXT("medical_kit")) return RiftInventoryPrivate::Profile(0.70f, 1.30f);
    if (ItemId == TEXT("medical_supplies")) return RiftInventoryPrivate::Profile(0.35f, 0.65f);
    if (ItemId == TEXT("replacement_part")) return RiftInventoryPrivate::Profile(1.15f, 0.90f);
    if (ItemId == TEXT("recovery_tool_t2")) return RiftInventoryPrivate::Profile(2.20f, 2.00f);
    if (ItemId == TEXT("recovery_tool_t3")) return RiftInventoryPrivate::Profile(4.50f, 4.20f);
    if (ItemId == TEXT("recovery_tool_t4")) return RiftInventoryPrivate::Profile(6.50f, 6.00f);
    if (ItemId == TEXT("breach_core")) return RiftInventoryPrivate::Profile(3.00f, 2.80f);
    return RiftInventoryPrivate::Profile(0.35f, 0.45f);
}

FRiftItemPhysicalProfile URiftInventoryRules::GetItemProfile(FName ItemId)
{
    return ResolveProfile(ItemId);
}

float URiftInventoryRules::GetInventoryMassKg(const ARiftPlayerCharacter* Player)
{
    if (!Player)
    {
        return 0.0f;
    }

    float Total = Player->Scrap * ResolveProfile(TEXT("scrap")).UnitMassKg;
    for (const TPair<FName, int32>& Pair : Player->Components)
    {
        Total += FMath::Max(0, Pair.Value) * ResolveProfile(Pair.Key).UnitMassKg;
    }
    return Total;
}

float URiftInventoryRules::GetInventoryVolumeL(const ARiftPlayerCharacter* Player)
{
    if (!Player)
    {
        return 0.0f;
    }

    float Total = Player->Scrap * ResolveProfile(TEXT("scrap")).UnitVolumeL;
    for (const TPair<FName, int32>& Pair : Player->Components)
    {
        Total += FMath::Max(0, Pair.Value) * ResolveProfile(Pair.Key).UnitVolumeL;
    }
    return Total;
}

float URiftInventoryRules::GetMaximumMassKg()
{
    return RiftInventoryPrivate::MaxMassKg;
}

float URiftInventoryRules::GetMaximumVolumeL()
{
    return RiftInventoryPrivate::MaxVolumeL;
}

bool URiftInventoryRules::CanAcceptItem(const ARiftPlayerCharacter* Player, FName ItemId, int32 Amount, FText& OutReason)
{
    if (!Player || Amount <= 0)
    {
        OutReason = FText::FromString(TEXT("Invalid inventory transfer"));
        return false;
    }

    const FRiftItemPhysicalProfile Profile = ResolveProfile(ItemId);
    const float NewMass = GetInventoryMassKg(Player) + Profile.UnitMassKg * Amount;
    const float NewVolume = GetInventoryVolumeL(Player) + Profile.UnitVolumeL * Amount;

    if (NewMass > RiftInventoryPrivate::MaxMassKg + KINDA_SMALL_NUMBER)
    {
        OutReason = FText::FromString(FString::Printf(
            TEXT("Too heavy: %.1f / %.1f kg"), NewMass, RiftInventoryPrivate::MaxMassKg));
        return false;
    }

    if (NewVolume > RiftInventoryPrivate::MaxVolumeL + KINDA_SMALL_NUMBER)
    {
        OutReason = FText::FromString(FString::Printf(
            TEXT("Pack full: %.1f / %.1f L"), NewVolume, RiftInventoryPrivate::MaxVolumeL));
        return false;
    }

    OutReason = FText::GetEmpty();
    return true;
}
