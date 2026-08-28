#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionTypes.h"
#include "RiftHumanoidNPC.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UPointLightComponent;
class UAnimSequenceBase;
class ARiftPlayerCharacter;

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftHumanoidNPC : public ACharacter
{
    GENERATED_BODY()

public:
    ARiftHumanoidNPC();
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|AI")
    TObjectPtr<UAIPerceptionComponent> Perception;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Combat")
    TObjectPtr<UPointLightComponent> WeaponMuzzleLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|AI")
    float Health = 55.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|AI")
    float PatrolRadius = 900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|AI")
    float WalkSpeed = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|AI")
    float CombatSpeed = 320.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Combat")
    float RangedDamage = 9.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Combat")
    float FireInterval = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Combat")
    float FireRange = 2600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Animation")
    bool bUseSingleNodeAnimationFallback = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Animation")
    TObjectPtr<UAnimSequenceBase> IdleAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Animation")
    TObjectPtr<UAnimSequenceBase> WalkAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Animation")
    TObjectPtr<UAnimSequenceBase> RunAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Animation")
    TObjectPtr<UAnimSequenceBase> PistolIdleAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Animation")
    TObjectPtr<UAnimSequenceBase> PistolShootAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Animation")
    TObjectPtr<UAnimSequenceBase> DeathAnimation;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|AI")
    bool bAlerted = false;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|AI")
    void AlertToLocation(FVector Location);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnAlerted(FVector Location);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnRangedAttack(ARiftPlayerCharacter* TargetPlayer);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnKilled();

protected:
    virtual void BeginPlay() override;

    UPROPERTY()
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    UPROPERTY()
    TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

    UPROPERTY()
    TObjectPtr<ARiftPlayerCharacter> PlayerTarget;

    FVector PatrolOrigin = FVector::ZeroVector;
    FVector InvestigationLocation = FVector::ZeroVector;
    float FireCooldown = 0.0f;
    float PatrolCooldown = 0.0f;
    bool bDead = false;
    bool bAttackAnimationLocked = false;
    TObjectPtr<UAnimSequenceBase> CurrentFallbackAnimation;

    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    void UpdateBehavior(float DeltaSeconds);
    void UpdateFallbackAnimation();
    void PickPatrolPoint();
    void FireAtPlayer();
    void EndAttackAnimation();
    void EndMuzzleFlash();
    void Die();

    FTimerHandle AttackAnimTimer;
    FTimerHandle MuzzleTimer;
};
