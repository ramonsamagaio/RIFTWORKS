#pragma once

#include "CoreMinimal.h"
#include "RiftSalvageFabrication.h"
#include "RiftCapacitySalvage.generated.h"

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftCapacitySalvageActor : public ARiftTieredSalvageActor
{
    GENERATED_BODY()

public:
    virtual void Interact_Implementation(ARiftPlayerCharacter* Player) override;

private:
    bool CanAcceptDismantleYield(ARiftPlayerCharacter* Player, FText& OutReason) const;
};
