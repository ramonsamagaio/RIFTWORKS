#pragma once

#include "CoreMinimal.h"
#include "RiftGameplayActors.h"
#include "RiftSalvageFabrication.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftTieredSalvageActor : public ARiftSalvageActor
{
    GENERATED_BODY()

public:
    ARiftTieredSalvageActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Salvage Progression", meta=(ClampMin="1", ClampMax="4"))
    int32 RecoveryTier = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Salvage Progression")
    bool bDismantleable = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Salvage Progression")
    int32 DismantleScrapYield = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Salvage Progression")
    FName DismantleComponentId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Salvage Progression")
    int32 DismantleComponentAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Salvage Progression")
    bool bMechanicallyRecovered = false;

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Salvage Progression")
    static int32 GetPlayerRecoveryToolTier(const ARiftPlayerCharacter* Player);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Salvage Progression")
    bool Dismantle(ARiftPlayerCharacter* Player);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Salvage Progression")
    void MarkMechanicallyRecovered();

private:
    bool MeetsToolRequirement(const ARiftPlayerCharacter* Player) const;
    void MarkRemovedFromPersistence(ARiftPlayerCharacter* Player) const;
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftRecoveryWinch : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftRecoveryWinch();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Recovery")
    TObjectPtr<UStaticMeshComponent> Frame;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Recovery")
    TObjectPtr<UStaticMeshComponent> Spool;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Recovery")
    TObjectPtr<UPointLightComponent> StatusLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Recovery", meta=(ClampMin="3", ClampMax="4"))
    int32 RecoveryCapacityTier = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Recovery")
    float TargetRange = 2600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Recovery")
    float PullForce = 420000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Recovery")
    float DockDistance = 230.0f;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Recovery")
    TObjectPtr<ARiftTieredSalvageActor> AttachedSalvage;

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Recovery")
    bool AttachTarget(ARiftTieredSalvageActor* Target);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Recovery")
    void ReleaseTarget();
};

UENUM(BlueprintType)
enum class ERiftFabricationRecipe : uint8
{
    Ammunition,
    Cable,
    Fasteners,
    StructuralPieces,
    MedicalKit,
    ReplacementPart
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftFabricator : public AActor, public IRiftInteractable
{
    GENERATED_BODY()

public:
    ARiftFabricator();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Fabrication")
    TObjectPtr<UStaticMeshComponent> Frame;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Fabrication")
    TObjectPtr<UStaticMeshComponent> WorkSurface;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Fabrication")
    TObjectPtr<UPointLightComponent> StatusLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Fabrication")
    ERiftFabricationRecipe SelectedRecipe = ERiftFabricationRecipe::Cable;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Fabrication")
    FText LastStatus;

    virtual FText GetInteractionText_Implementation() const override;
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Fabrication")
    void CycleRecipe(int32 Direction = 1);

    UFUNCTION(BlueprintCallable, Category="RIFTWORKS|Fabrication")
    bool Fabricate(ARiftPlayerCharacter* Player);

private:
    FString GetRecipeName() const;
    FString GetRecipeCostText() const;
};
