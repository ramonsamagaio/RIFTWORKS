#include "RiftHUD.h"

#include "RiftGameplayActors.h"
#include "RiftPlayerCharacter.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

void ARiftHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas)
    {
        return;
    }

    const float W = Canvas->SizeX;
    const float H = Canvas->SizeY;
    ARiftPlayerCharacter* Player = PlayerOwner ? Cast<ARiftPlayerCharacter>(PlayerOwner->GetPawn()) : nullptr;

    if (bDrawNativeFallback && Player)
    {
        UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
        const FLinearColor Primary(0.80f, 0.88f, 0.92f, 0.95f);
        const FLinearColor Dim(0.52f, 0.63f, 0.69f, 0.82f);
        const FLinearColor Warm(0.98f, 0.83f, 0.58f, 0.96f);

        float Generation = 0.0f;
        float Consumption = 0.0f;
        float Stored = 0.0f;
        for (TActorIterator<ARiftPowerDevice> It(GetWorld()); It; ++It)
        {
            if (!It->bEnabled)
            {
                continue;
            }
            if (It->Kind == ERiftPowerKind::Generator)
            {
                Generation += It->GenerationKW;
            }
            else if (It->Kind == ERiftPowerKind::Consumer && It->bPowered)
            {
                Consumption += It->ConsumptionKW;
            }
            else if (It->Kind == ERiftPowerKind::Battery)
            {
                Stored += It->ChargeKWh;
            }
        }

        const FString Status = FString::Printf(
            TEXT("RIFTWORKS  //  GRID DARK\nHP %.0f   FLASHLIGHT %.0f%%   SCRAP %d\nGRID %.1f kW GEN  /  %.1f kW LOAD   |   %.2f kWh STORED"),
            Player->Health,
            Player->FlashlightBattery,
            Player->Scrap,
            Generation,
            Consumption,
            Stored);
        DrawText(Status, Primary, SafeMargin, SafeMargin, Font, 1.0f, false);

        const float CX = W * 0.5f;
        const float CY = H * 0.5f;
        DrawLine(CX - 7.0f, CY, CX + 7.0f, CY, FLinearColor(0.9f, 0.95f, 1.0f, 0.65f), 1.0f);
        DrawLine(CX, CY - 7.0f, CX, CY + 7.0f, FLinearColor(0.9f, 0.95f, 1.0f, 0.65f), 1.0f);

        if (!Player->CurrentInteractionText.IsEmpty())
        {
            const FString Prompt = Player->CurrentInteractionText.ToString();
            float XL = 0.0f;
            float YL = 0.0f;
            GetTextSize(Prompt, XL, YL, Font, 1.15f);
            DrawText(Prompt, Warm, CX - XL * 0.5f, H * 0.70f, Font, 1.15f, false);
        }

        if (Player->CarriedSalvage)
        {
            const FString Cargo = FString::Printf(TEXT("CARRYING: %s   |   J DROP   H SECURE AT BASE"), *Player->CarriedSalvage->DisplayName.ToString());
            DrawText(Cargo, Warm, SafeMargin, H - 70.0f, Font, 1.0f, false);
        }
        else
        {
            DrawText(TEXT("F FLASHLIGHT   E INTERACT   SHIFT SPRINT   CTRL CROUCH   LMB FIRE"), Dim, SafeMargin, H - 48.0f, Font, 0.95f, false);
        }
    }

    BP_DrawRiftHUD(W, H);
}
