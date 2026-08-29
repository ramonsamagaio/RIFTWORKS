#pragma once

#include "CoreMinimal.h"
#include "RiftEngineeringJoint.h"
#include "RiftLogicNode.h"
#include "RiftPlayerCharacter.h"
#include "RiftProductionPlayer.generated.h"

UENUM(BlueprintType)
enum class ERiftUtilityBuildMode : uint8
{
    Button,
    ProximitySensor,
    Timer
};

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Engineering|Logic")
    ERiftUtilityBuildMode SelectedUtilityBuildMode = ERiftUtilityBuildMode::Button;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Engineering|Logic")
    TSubclassOf<ARiftLogicNode> LogicButtonClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Engineering|Logic")
    TSubclassOf<ARiftLogicNode> LogicSensorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Engineering|Logic")
    TSubclassOf<ARiftLogicNode> LogicTimerClass;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Engineering|Logic")
    TObjectPtr<ARiftLogicNode> LogicSelectionSource;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Engineering")
    void CycleEngineeringJointMode(int32 Direction = 1);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Engineering")
    void CancelEngineeringSelection();

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Engineering|Logic")
    void CycleUtilityBuildMode(int32 Direction = 1);

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Build")
    FString GetSelectedBuildCostText() const;

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Build")
    bool CanAffordSelectedBuildPiece() const;

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Engineering|Logic")
    FString GetUtilityBuildCostText() const;

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnStaminaChanged(float NewStamina, float MaximumStamina, bool bExhausted);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnEngineeringSelectionChanged(ARiftAssemblyPart* FirstPart, ERiftJointMode JointMode);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnEngineeringJointCreated(ARiftEngineeringJoint* Joint);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnLogicLinkCreated(ARiftLogicNode* Source, AActor* Receiver);

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
    AActor* TraceEngineeringActor(FHitResult* OutHit = nullptr) const;
    FString EngineeringModeName() const;
    void UpdateEngineeringPrompt();
    void ProductionBuildPlacePressed();
    bool ConsumeSelectedBuildCost();
    void UpdateBuildCostPrompt();
    void UtilityBuildNextPressed();
    void UtilityBuildPlacePressed();
    void LogicConnectPressed();
    FString UtilityModeName() const;
    TSubclassOf<ARiftLogicNode> GetSelectedUtilityClass() const;
    bool CanAffordUtility() const;
    bool ConsumeUtilityCost();
};
