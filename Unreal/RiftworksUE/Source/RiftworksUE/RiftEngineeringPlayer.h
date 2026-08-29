#pragma once

#include "CoreMinimal.h"
#include "RiftProductionPlayer.h"
#include "RiftEngineeringPlayer.generated.h"

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftEngineeringPlayerCharacter : public ARiftProductionPlayerCharacter
{
    GENERATED_BODY()

public:
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
};
