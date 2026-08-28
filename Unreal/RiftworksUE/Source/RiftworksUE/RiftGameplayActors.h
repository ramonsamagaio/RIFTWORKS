#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "RiftPlayerCharacter.h"
#include "RiftGameplayActors.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class USpotLightComponent;
class USphereComponent;
class ARiftPlayerCharacter;

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftSalvageActor : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftSalvageActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Salvage")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Salvage")
    FName ItemId = TEXT("scrap");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Salvage")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Salvage")
    int32 Amount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Salvage")
    bool bHeavy = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Salvage")
    float MassKg = 28.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Persistence")
    FString PersistentId;

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Salvage")
    void SetCarriedState(bool bCarried);
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftBaseBeacon : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftBaseBeacon();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UPointLightComponent> BeaconLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Base")
    TMap<FName, int32> Storage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Base")
    float AccessRadius = 1300.0f;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Base")
    void StoreItem(FName ItemId, int32 Amount);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Base")
    bool TakeItem(FName ItemId, int32 Amount);

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Base")
    bool IsPlayerInRange(const ARiftPlayerCharacter* Player) const;

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;
};

UENUM(BlueprintType)
enum class ERiftPowerKind : uint8
{
    Generator,
    Battery,
    Consumer
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftPowerDevice : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftPowerDevice();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UPointLightComponent> StatusLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USpotLightComponent> WorkLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Power")
    ERiftPowerKind Kind = ERiftPowerKind::Consumer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Power")
    FText DeviceName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Power")
    float GenerationKW = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Power")
    float ConsumptionKW = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Power")
    float CapacityKWh = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Power")
    float ChargeKWh = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Power")
    int32 Priority = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Power")
    bool bEnabled = true;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Power")
    bool bPowered = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Power")
    TArray<TObjectPtr<ARiftPowerDevice>> Links;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Power")
    void ConnectTo(ARiftPowerDevice* Other);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Power")
    void SetDeviceEnabled(bool bNewEnabled);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Power")
    void RecomputeLinkedGrid(float DeltaSeconds = 0.5f);

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Power")
    float GetPowerSignatureStrength() const;

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;

protected:
    float RecomputeTimer = 0.0f;
    void RefreshVisualState();
    void GatherNetwork(TSet<ARiftPowerDevice*>& OutNetwork);
};

UENUM(BlueprintType)
enum class ERiftBreachMode : uint8
{
    Repulsion,
    Attraction,
    Luminance,
    Gravity
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftBreachEmitter : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftBreachEmitter();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USphereComponent> Field;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UPointLightComponent> CoreLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Breach")
    ERiftBreachMode Mode = ERiftBreachMode::Repulsion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Breach")
    float Radius = 750.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Breach")
    float ForceStrength = 145000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Breach")
    bool bEnabled = true;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Breach")
    void SetSignal(bool bSignal);

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;
};

UENUM(BlueprintType)
enum class ERiftAssemblyPartType : uint8
{
    Platform,
    Beam,
    Wheel,
    MotorWheel
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftAssemblyPart : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftAssemblyPart();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMeshComponent> PhysicsMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    ERiftAssemblyPartType PartType = ERiftAssemblyPartType::Platform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    bool bMotorEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|FAS")
    float MotorTorque = 1800000.0f;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="RIFTWORKS|FAS")
    void ConfigurePart();

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|FAS")
    void SetSignal(bool bSignal);

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftColossus : public ACharacter
{
    GENERATED_BODY()

public:
    ARiftColossus();
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Colossus")
    float MoveSpeed = 105.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Colossus")
    float RouteRadius = 4600.0f;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Colossus")
    int32 HarpoonCount = 0;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Colossus")
    float HeadHP = 45.0f;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Colossus")
    float TorsoHP = 60.0f;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Colossus")
    float LegsHP = 70.0f;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Colossus")
    void RegisterHarpoon();

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnWeakpointDestroyed(FName Weakpoint);

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnColossusKilled();

protected:
    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> BodyBlocks;

    FVector RouteCenter = FVector::ZeroVector;
    float RouteAngle = 0.0f;
    bool bHeadDestroyed = false;
    bool bTorsoDestroyed = false;
    bool bLegsDestroyed = false;
    bool bDead = false;

    void DamageWeakpoint(FName Weakpoint, float DamageAmount);
    void Die();
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftHarpoonAnchor : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftHarpoonAnchor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Hunting")
    float Range = 9000.0f;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Hunting")
    TObjectPtr<ARiftColossus> AttachedColossus;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Hunting")
    void FireHarpoon();

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Hunting")
    void SetSignal(bool bSignal);

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;
};
