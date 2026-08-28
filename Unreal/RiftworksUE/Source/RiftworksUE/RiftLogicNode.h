#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiftPlayerCharacter.h"
#include "RiftLogicNode.generated.h"

class UPointLightComponent;
class USphereComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ERiftLogicMode : uint8
{
    ToggleButton,
    ProximitySensor,
    TimerPulse
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftLogicNode : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftLogicNode();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USphereComponent> Sensor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UPointLightComponent> Indicator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Logic")
    ERiftLogicMode Mode = ERiftLogicMode::ToggleButton;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Logic")
    TArray<TObjectPtr<AActor>> Receivers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Logic")
    bool bSignal = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Logic")
    float SensorRadius = 850.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Logic")
    float TimerSeconds = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Logic")
    bool bInvertOutput = false;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Logic")
    void SetSignal(bool bNewSignal);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Logic")
    void ToggleSignal();

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Logic")
    void ConnectReceiver(AActor* Receiver);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Logic")
    void DisconnectReceiver(AActor* Receiver);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Logic")
    void BroadcastSignal();

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;

protected:
    float TimerAccumulator = 0.0f;
    void RefreshIndicator();
    void SendSignal(AActor* Receiver, bool Value);
};
