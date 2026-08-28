#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RiftPersistence.generated.h"

class ARiftPlayerCharacter;

UCLASS()
class RIFTWORKSUE_API URiftSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    int32 Version = 1;

    UPROPERTY()
    int32 WorldSeed = 731942;

    UPROPERTY()
    FTransform PlayerTransform;

    UPROPERTY()
    float Health = 100.0f;

    UPROPERTY()
    float FlashlightBattery = 100.0f;

    UPROPERTY()
    bool bFlashlightOn = true;

    UPROPERTY()
    int32 Scrap = 0;

    UPROPERTY()
    TMap<FName, int32> Components;

    UPROPERTY()
    bool bHasBase = false;

    UPROPERTY()
    FTransform BaseTransform;

    UPROPERTY()
    TMap<FName, int32> BaseStorage;

    UPROPERTY()
    TArray<FString> RemovedSalvageIds;
};

UCLASS()
class RIFTWORKSUE_API URiftPersistenceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Save")
    FString SlotName = TEXT("RiftworksSlot0");

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Persistence")
    TSet<FString> RemovedSalvageIds;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Save")
    bool SaveRiftGame(ARiftPlayerCharacter* Player);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Save")
    bool LoadRiftGame(ARiftPlayerCharacter* Player);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Save")
    bool HasSaveGame() const;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Persistence")
    void MarkSalvageRemoved(const FString& PersistentId);

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Persistence")
    bool IsSalvageRemoved(const FString& PersistentId) const;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Save")
    void ClearRiftSave();
};
