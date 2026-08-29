#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiftPlayerCharacter.h"
#include "RiftPhaseField.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UPrimitiveComponent;

USTRUCT()
struct FRiftPhaseRestoreState
{
    GENERATED_BODY()

    ECollisionEnabled::Type CollisionEnabled = ECollisionEnabled::QueryAndPhysics;
    bool bGravityEnabled = true;
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftPhaseField : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftPhaseField();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Breach")
    TObjectPtr<UStaticMeshComponent> CoreMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Breach")
    TObjectPtr<USphereComponent> Field;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Breach")
    TObjectPtr<UPointLightComponent> CoreLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Breach")
    float Radius = 620.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Breach")
    bool bEnabled = true;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Breach")
    void SetSignal(bool bSignal);

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;

protected:
    virtual void BeginPlay() override;

    TMap<TWeakObjectPtr<UPrimitiveComponent>, FRiftPhaseRestoreState> PhasedComponents;

    void UpdatePhasedBodies();
    void RestoreComponent(UPrimitiveComponent* Primitive, const FRiftPhaseRestoreState& State);
    void RestoreAll();
};
