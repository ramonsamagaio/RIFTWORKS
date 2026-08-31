#pragma once

#include "CoreMinimal.h"
#include "RiftEngineeringPlayer.h"
#include "RiftSurvivalPlayer.generated.h"

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftSurvivalPlayerCharacter : public ARiftEngineeringPlayerCharacter
{
    GENERATED_BODY()

public:
    ARiftSurvivalPlayerCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
    virtual void BeginPlay() override;

private:
    void ToggleInventoryInput();
    void StabilizeFlashlight();
};
