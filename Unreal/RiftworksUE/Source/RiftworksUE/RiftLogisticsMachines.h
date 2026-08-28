#pragma once

#include "CoreMinimal.h"
#include "RiftGameplayActors.h"
#include "RiftLogisticsMachines.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftPoweredConveyor : public ARiftPowerDevice
{
    GENERATED_BODY()

public:
    ARiftPoweredConveyor();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Logistics")
    TObjectPtr<UStaticMeshComponent> Belt;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Logistics")
    TObjectPtr<UBoxComponent> TransportZone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Logistics")
    float BeltAcceleration = 520.0f;

    virtual FText GetInteractionText_Implementation() const override;
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftFreightLift : public ARiftPowerDevice
{
    GENERATED_BODY()

public:
    ARiftFreightLift();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Logistics")
    TObjectPtr<UStaticMeshComponent> Platform;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Logistics")
    TObjectPtr<UStaticMeshComponent> LeftRail;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Logistics")
    TObjectPtr<UStaticMeshComponent> RightRail;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Logistics")
    float TravelHeight = 920.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Logistics")
    float LiftSpeed = 165.0f;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Logistics")
    bool bTargetTop = false;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Logistics")
    bool bMoving = false;

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;

protected:
    FVector PlatformBottomLocation = FVector::ZeroVector;
    virtual void BeginPlay() override;
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftPhysicsCargoCart : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftPhysicsCargoCart();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Logistics")
    TObjectPtr<UStaticMeshComponent> Deck;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Logistics")
    TObjectPtr<UStaticMeshComponent> Handle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Logistics")
    float PushImpulse = 16500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Logistics")
    float CartMassKg = 72.0f;

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;
};
