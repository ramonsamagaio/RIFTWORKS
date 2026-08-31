#pragma once

#include "CoreMinimal.h"
#include "RiftHumanoidNPC.h"
#include "RiftAnimatedHumanoidNPC.generated.h"

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftAnimatedHumanoidNPC : public ARiftHumanoidNPC
{
    GENERATED_BODY()

public:
    ARiftAnimatedHumanoidNPC();
    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

private:
    void NormalizeVisualToCapsule();
    void GroundCapsuleToWorld();
    float GroundAuditTimer = 0.0f;
};
