#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RiftHUD.generated.h"

class ARiftLootContainer;
class ARiftPlayerCharacter;

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
    virtual void NotifyHitBoxRelease(FName BoxName) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|HUD")
    bool bDrawNativeFallback = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|HUD")
    float SafeMargin = 24.0f;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Inventory")
    bool bInventoryVisible = false;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Inventory")
    TObjectPtr<ARiftLootContainer> OpenContainerActor;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Inventory")
    void ToggleInventory();

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Inventory")
    void OpenContainer(ARiftLootContainer* Container);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Inventory")
    void CloseInventory();

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_DrawRiftHUD(float ScreenWidth, float ScreenHeight);

private:
    void DrawStatusHUD(ARiftPlayerCharacter* Player, float W, float H);
    void DrawInventoryHUD(ARiftPlayerCharacter* Player, float W, float H);
    void RefreshPlayerSlotOrder(ARiftPlayerCharacter* Player);
    void SetInventoryInputMode(bool bEnabled);
    int32 GetPlayerItemCount(const ARiftPlayerCharacter* Player, FName ItemId) const;
    bool TransferPlayerToContainer(ARiftPlayerCharacter* Player, int32 PlayerSlot, int32 ContainerSlot);
    bool TransferContainerToPlayer(ARiftPlayerCharacter* Player, int32 ContainerSlot, int32 PlayerSlot);
    bool ParseSlotName(FName BoxName, TCHAR& OutSide, int32& OutIndex) const;
    FLinearColor ItemColor(FName ItemId) const;

    TArray<FName> PlayerSlotOrder;
    FName DragSourceBox = NAME_None;
    FText InventoryMessage;
};
