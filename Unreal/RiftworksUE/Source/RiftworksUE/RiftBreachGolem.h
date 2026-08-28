#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Navigation/PathFollowingComponent.h"
#include "RiftBreachGolem.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;
class ARiftPlayerCharacter;

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftBreachGolem : public ACharacter
{
    GENERATED_BODY()

public:
    ARiftBreachGolem();
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Golem")
    float Health = 110.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Golem")
    float MoveSpeed = 190.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Golem")
    float DetectionRange = 2400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Golem")
    float AttackRange = 185.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Golem")
    float AttackDamage = 18.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Golem")
    TObjectPtr<UPointLightComponent> CoreLight;

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnGolemKilled();

protected:
    virtual void BeginPlay() override;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Torso;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Head;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> LeftArm;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> RightArm;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> LeftLeg;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> RightLeg;

    UPROPERTY()
    TObjectPtr<ARiftPlayerCharacter> TargetPlayer;

    float WalkPhase = 0.0f;
    float AttackCooldown = 0.0f;
    bool bDead = false;

    UStaticMeshComponent* MakeBlock(FName Name, FVector Location, FVector Scale);
    void Die();
};
