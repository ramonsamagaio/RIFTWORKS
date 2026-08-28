#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UObject/Interface.h"
#include "RiftPlayerCharacter.generated.h"

class UCameraComponent;
class USpotLightComponent;
class UPointLightComponent;
class UAnimSequenceBase;
class ARiftSalvageActor;
class ARiftBaseBeacon;
class ARiftAssemblyPart;

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

UENUM(BlueprintType)
enum class ERiftBuildPiece : uint8
{
    Platform,
    Beam,
    Wheel,
    MotorWheel
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ARiftPlayerCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Player")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Lighting")
    TObjectPtr<USpotLightComponent> Flashlight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Lighting")
    TObjectPtr<UPointLightComponent> MuzzleFlash;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Player")
    float Health = 100.0f;

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
    float RifleDamage = 28.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Combat")
    float RifleRange = 15000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Inventory")
    int32 Scrap = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Inventory")
    TMap<FName, int32> Components;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Logistics")
    TObjectPtr<ARiftSalvageActor> CarriedSalvage;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Interaction")
    FText CurrentInteractionText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Animation")
    bool bUseSingleNodeAnimationFallback = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Animation")
    TObjectPtr<UAnimSequenceBase> IdleAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Animation")
    TObjectPtr<UAnimSequenceBase> WalkAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Animation")
    TObjectPtr<UAnimSequenceBase> RunAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Animation")
    TObjectPtr<UAnimSequenceBase> CrouchAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Animation")
    TObjectPtr<UAnimSequenceBase> PistolShootAnimation;

    // --- Prototype construction mode. Native foundation, Blueprint-facing controls. ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Build")
    bool bBuildMode = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Build")
    ERiftBuildPiece SelectedBuildPiece = ERiftBuildPiece::Platform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Build")
    bool bBuildAnchored = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Build")
    float BuildDistance = 900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Build")
    float BuildGridSize = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Build")
    float BuildRotationStep = 15.0f;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Build")
    TObjectPtr<ARiftAssemblyPart> BuildPreview;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Lighting")
    void SetFlashlightEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Inventory")
    void AddComponentItem(FName ItemId, int32 Amount = 1);

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Inventory")
    int32 GetComponentCount(FName ItemId) const;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Inventory")
    bool ConsumeComponentItem(FName ItemId, int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Logistics")
    bool TryCarrySalvage(ARiftSalvageActor* Salvage);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Logistics")
    void DropHeavySalvage();

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Logistics")
    bool SecureHeavySalvage();

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Base")
    ARiftBaseBeacon* FindNearbyBase(float Radius = 1400.0f) const;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Build")
    void ToggleBuildMode();

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Build")
    void CycleBuildPiece(int32 Direction = 1);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Build")
    void RotateBuildPreview();

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Build")
    void ToggleBuildAnchor();

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Build")
    bool PlaceBuildPiece();

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnInventoryChanged();

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnFlashlightChanged(bool bEnabled, float BatteryPercent);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnWeaponFired(const FHitResult& Hit);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnCarriedSalvageChanged(ARiftSalvageActor* NewCarriedSalvage);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnDamaged(float NewHealth, float DamageAmount);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnDied();

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnBuildModeChanged(bool bEnabled, ERiftBuildPiece Piece, bool bAnchored);

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
    void DropHeavyPressed();
    void SecureHeavyPressed();
    void SavePressed();
    void LoadPressed();
    void BuildTogglePressed();
    void BuildNextPressed();
    void BuildPrevPressed();
    void BuildRotatePressed();
    void BuildAnchorPressed();
    void BuildPlacePressed();
    void UpdateInteractionTrace();
    void UpdateFallbackAnimation();
    void UpdateBuildPreview();
    void RecreateBuildPreview();
    void EndMuzzleFlash();
    void EndAttackAnimation();

    float BuildYaw = 0.0f;
    bool bAttackAnimationLocked = false;
    TObjectPtr<UAnimSequenceBase> CurrentFallbackAnimation;
    FTimerHandle MuzzleFlashTimer;
    FTimerHandle AttackAnimationTimer;
};
