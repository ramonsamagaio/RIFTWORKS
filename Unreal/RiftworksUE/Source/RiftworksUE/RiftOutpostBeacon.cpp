#include "RiftOutpostBeacon.h"

#include "Components/PointLightComponent.h"

ARiftOutpostBeacon::ARiftOutpostBeacon()
{
    OutpostName = FText::FromString(TEXT("Field Outpost"));
    AccessRadius = 950.0f;
    StorageCapacityUnits = 36;
    bOnline = true;

    if (Mesh)
    {
        Mesh->SetRelativeScale3D(FVector(0.42f, 0.42f, 1.15f));
    }
    if (BeaconLight)
    {
        BeaconLight->SetIntensity(760.0f);
        BeaconLight->SetAttenuationRadius(620.0f);
        BeaconLight->SetLightColor(FColor(136, 197, 255));
        BeaconLight->SetVolumetricScatteringIntensity(0.12f);
    }
    Tags.Add(TEXT("RiftOutpost"));
}

int32 ARiftOutpostBeacon::GetStoredUnits() const
{
    int32 Total = 0;
    for (const TPair<FName, int32>& Pair : Storage)
    {
        Total += FMath::Max(0, Pair.Value);
    }
    return Total;
}

int32 ARiftOutpostBeacon::GetFreeStorageUnits() const
{
    return FMath::Max(0, StorageCapacityUnits - GetStoredUnits());
}

bool ARiftOutpostBeacon::StoreAtOutpost(FName ItemId, int32 Amount)
{
    if (!bOnline || Amount <= 0 || GetFreeStorageUnits() < Amount)
    {
        return false;
    }
    StoreItem(ItemId, Amount);
    return true;
}

void ARiftOutpostBeacon::SetOutpostOnline(bool bNewOnline)
{
    bOnline = bNewOnline;
    if (BeaconLight)
    {
        BeaconLight->SetVisibility(bOnline, true);
        BeaconLight->SetIntensity(bOnline ? 760.0f : 0.0f);
    }
}

FText ARiftOutpostBeacon::GetInteractionText_Implementation() const
{
    return FText::FromString(FString::Printf(
        TEXT("[E] %s | %s | storage %d/%d | respawn node"),
        *OutpostName.ToString(),
        bOnline ? TEXT("ONLINE") : TEXT("OFFLINE"),
        GetStoredUnits(),
        StorageCapacityUnits));
}

void ARiftOutpostBeacon::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    if (!Player || !bOnline || !IsPlayerInRange(Player))
    {
        return;
    }

    int32 Free = GetFreeStorageUnits();
    if (Free <= 0)
    {
        Player->CurrentInteractionText = FText::FromString(TEXT("Outpost storage is full"));
        return;
    }

    if (Player->Scrap > 0 && Free > 0)
    {
        const int32 Transfer = FMath::Min(Player->Scrap, Free);
        StoreItem(TEXT("scrap"), Transfer);
        Player->Scrap -= Transfer;
        Free -= Transfer;
    }

    TArray<FName> Keys;
    Player->Components.GetKeys(Keys);
    for (const FName Key : Keys)
    {
        if (Free <= 0)
        {
            break;
        }
        int32* Count = Player->Components.Find(Key);
        if (!Count || *Count <= 0)
        {
            continue;
        }
        const int32 Transfer = FMath::Min(*Count, Free);
        StoreItem(Key, Transfer);
        *Count -= Transfer;
        Free -= Transfer;
    }

    for (auto It = Player->Components.CreateIterator(); It; ++It)
    {
        if (It.Value() <= 0)
        {
            It.RemoveCurrent();
        }
    }

    Player->BP_OnInventoryChanged();
    Player->CurrentInteractionText = FText::FromString(FString::Printf(
        TEXT("Outpost supplied | %d/%d storage used"), GetStoredUnits(), StorageCapacityUnits));
}
