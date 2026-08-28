#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiftPlayerCharacter.h"
#include "RiftEngineeringJoint.generated.h"

class UPhysicsConstraintComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ERiftJointMode : uint8
{
    Weld,
    Hinge,
    Slider,
    RopeWinch
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftEngineeringJoint : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftEngineeringJoint();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|FAS")
    TObjectPtr<UPhysicsConstraintComponent> Constraint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|FAS")
    TObjectPtr<UStaticMeshComponent> Marker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    ERiftJointMode Mode = ERiftJointMode::Weld;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    TObjectPtr<AActor> ActorA;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    TObjectPtr<AActor> ActorB;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    float MotorTargetRPM = 42.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    float MotorStrength = 250000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    float SliderTravel = 350.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    float SliderStrength = 180000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    float RopeLength = 650.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    float WinchSpeed = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    float MinimumRopeLength = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    bool bSignal = false;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="RIFTWORKS|FAS")
    bool AttachActors(AActor* NewActorA, AActor* NewActorB);

    UFUNCTION(BlueprintCallable, CallInEditor, Category="RIFTWORKS|FAS")
    void ConfigureConstraint();

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|FAS")
    void SetSignal(bool bNewSignal);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|FAS")
    void ReverseMotor();

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UPrimitiveComponent* FindPhysicsPrimitive(AActor* Actor) const;
};
