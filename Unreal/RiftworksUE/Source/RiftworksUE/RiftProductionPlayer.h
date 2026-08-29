#pragma once

#include "CoreMinimal.h"
#include "RiftEngineeringJoint.h"
#include "RiftPlayerCharacter.h"
#include "RiftProductionPlayer.generated.h"

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftProductionPlayerCharacter : public ARiftPlayerCharacter
{
    GENERATED_BODY()

public:
    ARiftProductionPlayerCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Movement|Stamina")
    float MaxStamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Movement|Stamina")
    float Stamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Movement|Stamina")
    float SprintDrainPerSecond = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Movement|Stamina")
    float StaminaRecoveryPerSecond = 14.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Movement|Stamina")
    float SprintRecoveryThreshold = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Camera")
    float SprintFOVBoost = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Camera")
    float CameraInterpSpeed = 11.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Camera")
    float WalkBobAmplitude = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Camera")
    float SprintBobAmplitude = 1.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Camera")
    float BobFrequency = 8.5f;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Movement|Stamina")
    bool bSprintExhausted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Engineering")
    ERiftJointMode SelectedJointMode = ERiftJointMode::Weld;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Engineering")
    float EngineeringTraceDistance = 1350.0f;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Engineering")
    TObjectPtr<ARiftAssemblyPart> EngineeringSelectionA;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Engineering")
    TObjectPtr<ARiftEngineeringJoint> LastCreatedJoint;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Engineering")
    void CycleEngineeringJointMode(int32 Direction = 1);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Engineering")
    void CancelEngineeringSelection();

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnStaminaChanged(float NewStamina, float MaximumStamina, bool bExhausted);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnEngineeringSelectionChanged(ARiftAssemblyPart* FirstPart, ERiftJointMode JointMode);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnEngineeringJointCreated(ARiftEngineeringJoint* Joint);

protected:
    virtual void BeginPlay() override;

    float SmoothedCameraHeight = 64.0f;
    float BobPhase = 0.0f;
    float LastReportedStamina = -1.0f;

    void UpdateStamina(float DeltaSeconds);
    void UpdateFirstPersonCameraMotion(float DeltaSeconds);
    void EngineeringConnectPressed();
    void EngineeringModeNextPressed();
    void EngineeringCancelPressed();
    ARiftAssemblyPart* TraceEngineeringPart(FHitResult* OutHit = nullptr) const;
    FString EngineeringModeName() const;
    void UpdateEngineeringPrompt();
};
