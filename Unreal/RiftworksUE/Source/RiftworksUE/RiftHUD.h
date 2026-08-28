#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RiftHUD.generated.h"

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|HUD")
    bool bDrawNativeFallback = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|HUD")
    float SafeMargin = 24.0f;

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_DrawRiftHUD(float ScreenWidth, float ScreenHeight);
};
