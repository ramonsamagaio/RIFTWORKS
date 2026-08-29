#pragma once

#include "CoreMinimal.h"
#include "RiftGameplayActors.h"
#include "RiftMannequinColossus.generated.h"

class UAnimSequenceBase;
class UBoxComponent;

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftMannequinColossus : public ARiftColossus
{
    GENERATED_BODY()

public:
    ARiftMannequinColossus();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Colossus|Animation")
    TObjectPtr<UAnimSequenceBase> IdleAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Colossus|Animation")
    TObjectPtr<UAnimSequenceBase> WalkAnimation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Colossus|Weakpoints")
    TObjectPtr<UBoxComponent> LegsCollision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Colossus|Weakpoints")
    TObjectPtr<UBoxComponent> TorsoCollision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Colossus|Weakpoints")
    TObjectPtr<UBoxComponent> HeadCollision;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Colossus")
    float VisualScale = 16.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Colossus|Environment")
    float EnvironmentInteractionRadius = 980.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Colossus|Environment")
    float EnvironmentBreakRadius = 560.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Colossus|Environment")
    float EnvironmentImpulse = 185000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Colossus|Environment")
    float EnvironmentPulseInterval = 0.28f;

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnEnvironmentImpact(AActor* AffectedActor);

protected:
    virtual void BeginPlay() override;

    FVector PrototypeRouteCenter = FVector::ZeroVector;
    float PrototypeRouteAngle = 0.0f;
    float EnvironmentPulseTimer = 0.0f;
    TObjectPtr<UAnimSequenceBase> CurrentAnimation;

    bool IsAnimationCompatible(const UAnimSequenceBase* Animation) const;
    void NormalizeVisualToCapsule();
    void GroundCapsuleToWorld();
    void UpdatePrototypeAnimation();
    void PulseEnvironment();
};
