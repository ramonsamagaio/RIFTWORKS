#include "RiftHUD.h"

#include "RiftGameplayActors.h"
#include "RiftInventoryRules.h"
#include "RiftOutpostBeacon.h"
#include "RiftPlayerCharacter.h"
#include "RiftProductionPlayer.h"
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
        float Signature = 0.0f;
        for (TActorIterator<ARiftPowerDevice> It(GetWorld()); It; ++It)
        {
            if (!It->bEnabled)
            {
                continue;
            }
            Signature += It->GetPowerSignatureStrength();
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

        int32 OnlineOutposts = 0;
        int32 OutpostStoredUnits = 0;
        for (TActorIterator<ARiftOutpostBeacon> It(GetWorld()); It; ++It)
        {
            if (It->bOnline)
            {
                ++OnlineOutposts;
            }
            OutpostStoredUnits += It->GetStoredUnits();
        }

        const float InventoryMass = URiftInventoryRules::GetInventoryMassKg(Player);
        const float InventoryVolume = URiftInventoryRules::GetInventoryVolumeL(Player);
        const ARiftProductionPlayerCharacter* ProductionPlayer = Cast<ARiftProductionPlayerCharacter>(Player);
        const float Stamina = ProductionPlayer ? ProductionPlayer->Stamina : 100.0f;
        const float MaxStamina = ProductionPlayer ? ProductionPlayer->MaxStamina : 100.0f;
        const FString Status = FString::Printf(
            TEXT("RIFTWORKS\nHP %.0f   STAM %.0f/%.0f   LIGHT %.0f%%   SCRAP %d\nPACK %.1f/%.0f kg   %.1f/%.0f L\nGRID %.1f kW GEN / %.1f kW LOAD / %.2f kWh   SIG %.1f\nOUTPOSTS %d ONLINE   %d STORED"),
            Player->Health,
            Stamina,
            MaxStamina,
            Player->FlashlightBattery,
            Player->Scrap,
            InventoryMass,
            URiftInventoryRules::GetMaximumMassKg(),
            InventoryVolume,
            URiftInventoryRules::GetMaximumVolumeL(),
            Generation,
            Consumption,
            Stored,
            Signature,
            OnlineOutposts,
            OutpostStoredUnits);
        DrawText(Status, Primary, SafeMargin, SafeMargin, Font, 1.0f, false);

        const float CX = W * 0.5f;
        const float CY = H * 0.5f;
        const FLinearColor Cross = Player->bBuildMode ? Warm : FLinearColor(0.9f, 0.95f, 1.0f, 0.65f);
        DrawLine(CX - 7.0f, CY, CX + 7.0f, CY, Cross, 1.0f);
        DrawLine(CX, CY - 7.0f, CX, CY + 7.0f, Cross, 1.0f);

        if (!Player->CurrentInteractionText.IsEmpty())
        {
            const FString Prompt = Player->CurrentInteractionText.ToString();
            float XL = 0.0f;
            float YL = 0.0f;
            GetTextSize(Prompt, XL, YL, Font, Player->bBuildMode ? 1.0f : 1.15f);
            DrawText(Prompt, Warm, CX - XL * 0.5f, H * 0.70f, Font, Player->bBuildMode ? 1.0f : 1.15f, false);
        }

        if (Player->CarriedSalvage)
        {
            const FString Cargo = FString::Printf(TEXT("CARRYING: %s   |   J DROP   H SECURE AT BASE"), *Player->CarriedSalvage->DisplayName.ToString());
            DrawText(Cargo, Warm, SafeMargin, H - 70.0f, Font, 1.0f, false);
        }
        else if (Player->bBuildMode)
        {
            const FString BuildHelp = ProductionPlayer
                ? FString::Printf(TEXT("BUILD   RMB PLACE   WHEEL PIECE   R ROTATE   Q ANCHOR   B EXIT   |   %s"), *ProductionPlayer->GetSelectedBuildCostText())
                : TEXT("BUILD MODE   RMB PLACE   WHEEL PIECE   R ROTATE   Q ANCHOR   B EXIT");
            DrawText(BuildHelp, Warm, SafeMargin, H - 48.0f, Font, 0.95f, false);
        }
        else if (ProductionPlayer)
        {
            const FString UtilityHelp = FString::Printf(
                TEXT("G JOINT   T JOINT TYPE   C UTILITY   MMB PLACE %s (%s)   Y SIGNAL LINK   |   F LIGHT  E USE  B BUILD"),
                *StaticEnum<ERiftUtilityBuildMode>()->GetNameStringByValue(static_cast<int64>(ProductionPlayer->SelectedUtilityBuildMode)),
                *ProductionPlayer->GetUtilityBuildCostText());
            DrawText(UtilityHelp, Dim, SafeMargin, H - 48.0f, Font, 0.86f, false);
        }
        else
        {
            DrawText(TEXT("F LIGHT   E USE   SHIFT SPRINT   CTRL CROUCH   LMB FIRE   B BUILD"), Dim, SafeMargin, H - 48.0f, Font, 0.90f, false);
        }
    }

    BP_DrawRiftHUD(W, H);
}
