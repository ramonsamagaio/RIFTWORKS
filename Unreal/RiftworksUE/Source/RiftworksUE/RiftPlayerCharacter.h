#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UObject/Interface.h"
#include "RiftPlayerCharacter.generated.h"

class UCameraComponent;
class USpotLightComponent;
class UPointLightComponent;

UINTERFACE(BlueprintType)
class RIFTWORKSUE_API URiftInteractable : public UInterface
{
    GENERATED_BODY()
};

class RIFTWORKSUE_API IRiftInteractable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RIFTWORKS|Interaction")
    FText GetInteractionText() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RIFTWORKS|Interaction")
    void Interact(class ARiftPlayerCharacter* Player);
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ARiftPlayerCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Player")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Lighting")
    TObjectPtr<USpotLightComponent> Flashlight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Lighting")
    TObjectPtr<UPointLightComponent> MuzzleFlash;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Movement")
    float WalkSpeed = 420.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Movement")
    float SprintSpeed = 680.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Camera")
    float FieldOfView = 92.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Lighting")
    float FlashlightBattery = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Lighting")
    float FlashlightDrainPerSecond = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Lighting")
    bool bFlashlightOn = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Combat")
    float RifleDamage = 22.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Combat")
    float RifleRange = 15000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Inventory")
    int32 Scrap = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Inventory")
    TMap<FName, int32> Components;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Interaction")
    FText CurrentInteractionText;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Lighting")
    void SetFlashlightEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Inventory")
    void AddComponentItem(FName ItemId, int32 Amount = 1);

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Inventory")
    int32 GetComponentCount(FName ItemId) const;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Inventory")
    bool ConsumeComponentItem(FName ItemId, int32 Amount = 1);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnInventoryChanged();

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnFlashlightChanged(bool bEnabled, float BatteryPercent);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnWeaponFired(const FHitResult& Hit);

protected:
    virtual void BeginPlay() override;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void StartSprint();
    void StopSprint();
    void ToggleFlashlight();
    void InteractPressed();
    void FirePressed();
    void ToggleCrouch();
    void UpdateInteractionTrace();
    void EndMuzzleFlash();

    FTimerHandle MuzzleFlashTimer;
};
