#pragma once

#include "CoreMinimal.h"
#include "RiftGameplayActors.h"
#include "RiftOutpostBeacon.generated.h"

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftOutpostBeacon : public ARiftBaseBeacon
{
    GENERATED_BODY()

public:
    ARiftOutpostBeacon();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Outpost")
    FText OutpostName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Outpost")
    int32 StorageCapacityUnits = 36;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Outpost")
    bool bOnline = true;

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Outpost")
    int32 GetStoredUnits() const;

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Outpost")
    int32 GetFreeStorageUnits() const;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Outpost")
    bool StoreAtOutpost(FName ItemId, int32 Amount);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Outpost")
    void SetOutpostOnline(bool bNewOnline);

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;
};
