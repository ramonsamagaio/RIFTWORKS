#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RiftCrawlerCreature.generated.h"

class UStaticMeshComponent;
class ARiftPlayerCharacter;

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftCrawlerCreature : public ACharacter
{
    GENERATED_BODY()

public:
    ARiftCrawlerCreature();
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Creature")
    float Health = 42.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Creature")
    float MoveSpeed = 245.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Creature")
    float DetectionRange = 1850.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Creature")
    float AttackRange = 135.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Creature")
    float AttackDamage = 11.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Creature")
    float AttackInterval = 1.05f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Creature")
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Creature")
    TObjectPtr<UStaticMeshComponent> Head;

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnCrawlerKilled();

protected:
    virtual void BeginPlay() override;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> Legs;

    UPROPERTY()
    TObjectPtr<ARiftPlayerCharacter> TargetPlayer;

    float GaitPhase = 0.0f;
    float AttackCooldown = 0.0f;
    bool bDead = false;

    UStaticMeshComponent* MakePart(FName Name, FVector Location, FVector Scale, FRotator Rotation = FRotator::ZeroRotator);
    void UpdateProceduralGait(float DeltaSeconds);
    void Die();
};
