#include "RiftPersistence.h"

#include "RiftGameplayActors.h"
#include "RiftPlayerCharacter.h"
#include "RiftWorldDirector.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

bool URiftPersistenceSubsystem::SaveRiftGame(ARiftPlayerCharacter* Player)
{
    if (!Player || !GetWorld())
    {
        return false;
    }

    URiftSaveGame* Save = Cast<URiftSaveGame>(UGameplayStatics::CreateSaveGameObject(URiftSaveGame::StaticClass()));
    if (!Save)
    {
        return false;
    }

    Save->PlayerTransform = Player->GetActorTransform();
    Save->Health = Player->Health;
    Save->FlashlightBattery = Player->FlashlightBattery;
    Save->bFlashlightOn = Player->bFlashlightOn;
    Save->Scrap = Player->Scrap;
    Save->Components = Player->Components;

    for (TActorIterator<ARiftWorldDirector> It(GetWorld()); It; ++It)
    {
        Save->WorldSeed = It->WorldSeed;
        break;
    }

    for (TActorIterator<ARiftBaseBeacon> It(GetWorld()); It; ++It)
    {
        Save->bHasBase = true;
        Save->BaseTransform = It->GetActorTransform();
        Save->BaseStorage = It->Storage;
        break;
    }

    Save->RemovedSalvageIds.Reset();
    for (const FString& Id : RemovedSalvageIds)
    {
        Save->RemovedSalvageIds.Add(Id);
    }

    return UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
}

bool URiftPersistenceSubsystem::LoadRiftGame(ARiftPlayerCharacter* Player)
{
    if (!Player || !GetWorld() || !HasSaveGame())
    {
        return false;
    }

    URiftSaveGame* Save = Cast<URiftSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    if (!Save)
    {
        return false;
    }

    ARiftWorldDirector* WorldDirector = nullptr;
    for (TActorIterator<ARiftWorldDirector> It(GetWorld()); It; ++It)
    {
        WorldDirector = *It;
        break;
    }
    if (WorldDirector)
    {
        const bool bSeedChanged = WorldDirector->WorldSeed != Save->WorldSeed;
        WorldDirector->WorldSeed = Save->WorldSeed;
        if (bSeedChanged)
        {
            WorldDirector->RegenerateVisibleWorld();
        }
    }

    Player->SetActorTransform(Save->PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
    Player->Health = Save->Health;
    Player->FlashlightBattery = Save->FlashlightBattery;
    Player->Scrap = Save->Scrap;
    Player->Components = Save->Components;
    Player->SetFlashlightEnabled(Save->bFlashlightOn && Save->FlashlightBattery > 0.0f);
    Player->BP_OnInventoryChanged();

    RemovedSalvageIds.Reset();
    for (const FString& Id : Save->RemovedSalvageIds)
    {
        RemovedSalvageIds.Add(Id);
    }

    ARiftBaseBeacon* Base = nullptr;
    for (TActorIterator<ARiftBaseBeacon> It(GetWorld()); It; ++It)
    {
        Base = *It;
        break;
    }
    if (Save->bHasBase)
    {
        if (!Base)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Base = GetWorld()->SpawnActor<ARiftBaseBeacon>(ARiftBaseBeacon::StaticClass(), Save->BaseTransform, Params);
        }
        if (Base)
        {
            Base->SetActorTransform(Save->BaseTransform, false, nullptr, ETeleportType::TeleportPhysics);
            Base->Storage = Save->BaseStorage;
        }
    }

    return true;
}

bool URiftPersistenceSubsystem::HasSaveGame() const
{
    return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

void URiftPersistenceSubsystem::MarkSalvageRemoved(const FString& PersistentId)
{
    if (!PersistentId.IsEmpty())
    {
        RemovedSalvageIds.Add(PersistentId);
    }
}

bool URiftPersistenceSubsystem::IsSalvageRemoved(const FString& PersistentId) const
{
    return !PersistentId.IsEmpty() && RemovedSalvageIds.Contains(PersistentId);
}

void URiftPersistenceSubsystem::ClearRiftSave()
{
    RemovedSalvageIds.Reset();
    if (HasSaveGame())
    {
        UGameplayStatics::DeleteGameInSlot(SlotName, 0);
    }
}
