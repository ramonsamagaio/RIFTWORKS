#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiftPlayerCharacter.h"
#include "RiftLootContainer.generated.h"

class USceneComponent;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FRiftLootStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Loot")
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Loot")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Loot")
    int32 Amount = 0;
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftLootContainer : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftLootContainer();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Loot")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Loot")
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Loot")
    TObjectPtr<UStaticMeshComponent> Lid;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Loot")
    TObjectPtr<UStaticMeshComponent> BandA;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Loot")
    TObjectPtr<UStaticMeshComponent> BandB;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Loot")
    TObjectPtr<UStaticMeshComponent> Handle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Loot")
    FText ContainerName = FText::FromString(TEXT("Storage Crate"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Loot", meta=(ClampMin="1", ClampMax="30"))
    int32 CapacitySlots = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Loot", meta=(ClampMin="1", ClampMax="4"))
    int32 LootTier = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Loot")
    int32 LootSeed = 1337;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Loot")
    bool bSeedDefaultLoot = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Loot")
    TArray<FRiftLootStack> Items;

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Loot")
    void SeedLoot();

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Loot")
    bool AddItem(FName ItemId, int32 Amount, const FText& DisplayName);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Loot")
    bool RemoveItemAt(int32 SlotIndex, int32 Amount, FRiftLootStack& OutRemoved);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Loot")
    bool SwapSlots(int32 SlotA, int32 SlotB);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Loot")
    void SetContainerOpen(bool bOpen);

protected:
    virtual void BeginPlay() override;

private:
    void ApplyRuntimeMaterials();
    void AddSeeded(FRandomStream& Stream, FName ItemId, const TCHAR* Label, int32 MinAmount, int32 MaxAmount);
    bool bOpenVisual = false;
};
