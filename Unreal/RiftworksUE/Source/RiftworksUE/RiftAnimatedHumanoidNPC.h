#pragma once

#include "CoreMinimal.h"
#include "RiftHumanoidNPC.h"
#include "RiftAnimatedHumanoidNPC.generated.h"

class UAnimSequenceBase;

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
    void LoadColossusAnimationFamily();
    void NormalizeVisualToCapsule();
    void GroundCapsuleToWorld();
    void UpdateColossusStyleAnimation();
    bool IsRuntimeAnimationCompatible(const UAnimSequenceBase* Animation) const;

    TObjectPtr<UAnimSequenceBase> RuntimeAnimation = nullptr;
};
