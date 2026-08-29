#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RiftInventoryRules.generated.h"

class ARiftPlayerCharacter;

USTRUCT(BlueprintType)
struct FRiftItemPhysicalProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Inventory")
    float UnitMassKg = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Inventory")
    float UnitVolumeL = 0.35f;
};

UCLASS()
class RIFTWORKSUE_API URiftInventoryRules : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Inventory")
    static FRiftItemPhysicalProfile GetItemProfile(FName ItemId);

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Inventory")
    static float GetInventoryMassKg(const ARiftPlayerCharacter* Player);

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Inventory")
    static float GetInventoryVolumeL(const ARiftPlayerCharacter* Player);

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Inventory")
    static float GetMaximumMassKg();

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Inventory")
    static float GetMaximumVolumeL();

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Inventory")
    static bool CanAcceptItem(const ARiftPlayerCharacter* Player, FName ItemId, int32 Amount, FText& OutReason);

private:
    static FRiftItemPhysicalProfile ResolveProfile(FName ItemId);
};
