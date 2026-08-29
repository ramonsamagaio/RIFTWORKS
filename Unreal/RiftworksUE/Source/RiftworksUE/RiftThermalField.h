#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiftPlayerCharacter.h"
#include "RiftThermalField.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class ACharacter;

UENUM(BlueprintType)
enum class ERiftTemperatureFieldMode : uint8
{
    Thermal,
    Cryo
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftTemperatureField : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftTemperatureField();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Breach")
    TObjectPtr<UStaticMeshComponent> CoreMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Breach")
    TObjectPtr<USphereComponent> Field;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Breach")
    TObjectPtr<UPointLightComponent> CoreLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Breach")
    ERiftTemperatureFieldMode Mode = ERiftTemperatureFieldMode::Thermal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Breach")
    float Radius = 720.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Breach")
    float ThermalLiftForce = 115000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Breach")
    float ThermalDamagePerSecond = 7.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Breach")
    float CryoDragForce = 95000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Breach")
    float CryoCharacterSpeed = 155.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Breach")
    bool bEnabled = true;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Breach")
    void SetSignal(bool bSignal);

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;

protected:
    virtual void BeginPlay() override;

    TMap<TWeakObjectPtr<ACharacter>, float> CryoOriginalSpeeds;

    void RefreshVisuals();
    void ApplyThermal(float DeltaSeconds, const TArray<AActor*>& Overlaps);
    void ApplyCryo(float DeltaSeconds, const TArray<AActor*>& Overlaps);
    void RestoreReleasedCryoCharacters(const TSet<TWeakObjectPtr<ACharacter>>& CurrentlyAffected);
};
