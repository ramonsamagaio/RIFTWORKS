#include "RiftHUD.h"

#include "RiftGameplayActors.h"
#include "RiftInventoryRules.h"
#include "RiftLootContainer.h"
#include "RiftOutpostBeacon.h"
#include "RiftPlayerCharacter.h"
#include "RiftProductionPlayer.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

namespace RiftHUDPrivate
{
    constexpr int32 PlayerSlots = 30;
    constexpr int32 Columns = 5;
    constexpr float SlotSize = 62.0f;
    constexpr float Gap = 8.0f;

    FString FriendlyItemName(FName ItemId)
    {
        FString Result = ItemId.ToString().Replace(TEXT("_"), TEXT(" "));
        if (!Result.IsEmpty())
        {
            Result[0] = FChar::ToUpper(Result[0]);
        }
        return Result;
    }
}

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
        DrawStatusHUD(Player, W, H);
    }

    if (bInventoryVisible && Player)
    {
        DrawInventoryHUD(Player, W, H);
    }

    BP_DrawRiftHUD(W, H);
}

void ARiftHUD::DrawStatusHUD(ARiftPlayerCharacter* Player, float W, float H)
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
        if (It->Kind == ERiftPowerKind::Generator) Generation += It->GenerationKW;
        else if (It->Kind == ERiftPowerKind::Consumer && It->bPowered) Consumption += It->ConsumptionKW;
        else if (It->Kind == ERiftPowerKind::Battery) Stored += It->ChargeKWh;
    }

    int32 OnlineOutposts = 0;
    for (TActorIterator<ARiftOutpostBeacon> It(GetWorld()); It; ++It)
    {
        OnlineOutposts += It->bOnline ? 1 : 0;
    }

    const float InventoryMass = URiftInventoryRules::GetInventoryMassKg(Player);
    const float InventoryVolume = URiftInventoryRules::GetInventoryVolumeL(Player);
    const ARiftProductionPlayerCharacter* ProductionPlayer = Cast<ARiftProductionPlayerCharacter>(Player);
    const float Stamina = ProductionPlayer ? ProductionPlayer->Stamina : 100.0f;
    const float MaxStamina = ProductionPlayer ? ProductionPlayer->MaxStamina : 100.0f;

    const FString Status = FString::Printf(
        TEXT("RIFTWORKS\nHP %.0f   STAM %.0f/%.0f   LIGHT %.0f%%\nPACK %.1f/%.0f kg   %.1f/%.0f L   OUTPOSTS %d\nGRID %.1f kW / %.1f kW   %.2f kWh"),
        Player->Health, Stamina, MaxStamina, Player->FlashlightBattery,
        InventoryMass, URiftInventoryRules::GetMaximumMassKg(),
        InventoryVolume, URiftInventoryRules::GetMaximumVolumeL(), OnlineOutposts,
        Generation, Consumption, Stored);
    DrawText(Status, Primary, SafeMargin, SafeMargin, Font, 1.0f, false);

    const float CX = W * 0.5f;
    const float CY = H * 0.5f;
    if (!bInventoryVisible)
    {
        const FLinearColor Cross = Player->bBuildMode ? Warm : FLinearColor(0.9f, 0.95f, 1.0f, 0.65f);
        DrawLine(CX - 7.0f, CY, CX + 7.0f, CY, Cross, 1.0f);
        DrawLine(CX, CY - 7.0f, CX, CY + 7.0f, Cross, 1.0f);
    }

    if (!bInventoryVisible && !Player->CurrentInteractionText.IsEmpty())
    {
        const FString Prompt = Player->CurrentInteractionText.ToString();
        float XL = 0.0f;
        float YL = 0.0f;
        GetTextSize(Prompt, XL, YL, Font, 1.1f);
        DrawText(Prompt, Warm, CX - XL * 0.5f, H * 0.70f, Font, 1.1f, false);
    }

    if (!bInventoryVisible)
    {
        DrawText(TEXT("TAB / I  INVENTORY   F LIGHT   E USE   SHIFT SPRINT   LMB FIRE   B BUILD"), Dim, SafeMargin, H - 48.0f, Font, 0.88f, false);
    }
}

void ARiftHUD::RefreshPlayerSlotOrder(ARiftPlayerCharacter* Player)
{
    if (!Player)
    {
        return;
    }
    if (PlayerSlotOrder.Num() != RiftHUDPrivate::PlayerSlots)
    {
        PlayerSlotOrder.SetNum(RiftHUDPrivate::PlayerSlots);
    }

    TSet<FName> Existing;
    if (Player->Scrap > 0)
    {
        Existing.Add(TEXT("scrap"));
    }
    for (const TPair<FName, int32>& Pair : Player->Components)
    {
        if (Pair.Value > 0)
        {
            Existing.Add(Pair.Key);
        }
    }

    for (FName& Slot : PlayerSlotOrder)
    {
        if (!Slot.IsNone() && !Existing.Contains(Slot))
        {
            Slot = NAME_None;
        }
    }

    for (const FName ItemId : Existing)
    {
        if (PlayerSlotOrder.Contains(ItemId))
        {
            continue;
        }
        const int32 Empty = PlayerSlotOrder.IndexOfByKey(NAME_None);
        if (Empty != INDEX_NONE)
        {
            PlayerSlotOrder[Empty] = ItemId;
        }
    }
}

int32 ARiftHUD::GetPlayerItemCount(const ARiftPlayerCharacter* Player, FName ItemId) const
{
    if (!Player || ItemId.IsNone())
    {
        return 0;
    }
    if (ItemId == TEXT("scrap"))
    {
        return Player->Scrap;
    }
    return Player->GetComponentCount(ItemId);
}

FLinearColor ARiftHUD::ItemColor(FName ItemId) const
{
    const uint32 Hash = GetTypeHash(ItemId);
    const float R = 0.22f + ((Hash >> 0) & 0xFF) / 255.0f * 0.38f;
    const float G = 0.24f + ((Hash >> 8) & 0xFF) / 255.0f * 0.38f;
    const float B = 0.28f + ((Hash >> 16) & 0xFF) / 255.0f * 0.42f;
    return FLinearColor(R, G, B, 0.96f);
}

void ARiftHUD::DrawInventoryHUD(ARiftPlayerCharacter* Player, float W, float H)
{
    RefreshPlayerSlotOrder(Player);
    UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
    const FLinearColor Backdrop(0.006f, 0.010f, 0.014f, 0.84f);
    const FLinearColor Panel(0.025f, 0.035f, 0.042f, 0.97f);
    const FLinearColor SlotEmpty(0.055f, 0.070f, 0.078f, 0.96f);
    const FLinearColor Text(0.82f, 0.89f, 0.91f, 1.0f);
    const FLinearColor Accent(0.93f, 0.69f, 0.32f, 1.0f);

    DrawRect(Backdrop, 0.0f, 0.0f, W, H);

    const float PanelW = 430.0f;
    const float PanelH = 560.0f;
    const float GapBetween = 34.0f;
    const float TotalW = PanelW * 2.0f + GapBetween;
    const float LeftX = (W - TotalW) * 0.5f;
    const float RightX = LeftX + PanelW + GapBetween;
    const float TopY = FMath::Max(70.0f, (H - PanelH) * 0.5f);

    DrawRect(Panel, LeftX, TopY, PanelW, PanelH);
    DrawRect(Panel, RightX, TopY, PanelW, PanelH);
    DrawText(TEXT("BACKPACK"), Accent, LeftX + 24.0f, TopY + 18.0f, Font, 1.25f, false);
    DrawText(OpenContainerActor ? OpenContainerActor->ContainerName.ToString().ToUpper() : TEXT("NEARBY STORAGE"), Accent, RightX + 24.0f, TopY + 18.0f, Font, 1.25f, false);

    const float Mass = URiftInventoryRules::GetInventoryMassKg(Player);
    const float Volume = URiftInventoryRules::GetInventoryVolumeL(Player);
    DrawText(FString::Printf(TEXT("%.1f / %.0f kg     %.1f / %.0f L"), Mass, URiftInventoryRules::GetMaximumMassKg(), Volume, URiftInventoryRules::GetMaximumVolumeL()),
        Text, LeftX + 24.0f, TopY + 47.0f, Font, 0.88f, false);

    const float StartY = TopY + 90.0f;
    const float StartXLeft = LeftX + 31.0f;
    const float StartXRight = RightX + 31.0f;

    auto DrawSlot = [&](float X, float Y, FName BoxName, FName ItemId, int32 Count, const FString& OverrideName)
    {
        DrawRect(SlotEmpty, X, Y, RiftHUDPrivate::SlotSize, RiftHUDPrivate::SlotSize);
        AddHitBox(FVector2D(X, Y), FVector2D(RiftHUDPrivate::SlotSize, RiftHUDPrivate::SlotSize), BoxName, true, 0);
        if (!ItemId.IsNone() && Count > 0)
        {
            DrawRect(ItemColor(ItemId), X + 4.0f, Y + 4.0f, RiftHUDPrivate::SlotSize - 8.0f, RiftHUDPrivate::SlotSize - 8.0f);
            FString Label = OverrideName.IsEmpty() ? RiftHUDPrivate::FriendlyItemName(ItemId) : OverrideName;
            if (Label.Len() > 10) Label = Label.Left(10);
            DrawText(Label, Text, X + 7.0f, Y + 9.0f, Font, 0.72f, false);
            DrawText(FString::Printf(TEXT("x%d"), Count), FLinearColor::White, X + 7.0f, Y + 38.0f, Font, 0.82f, false);
        }
    };

    for (int32 Index = 0; Index < RiftHUDPrivate::PlayerSlots; ++Index)
    {
        const int32 Row = Index / RiftHUDPrivate::Columns;
        const int32 Col = Index % RiftHUDPrivate::Columns;
        const float X = StartXLeft + Col * (RiftHUDPrivate::SlotSize + RiftHUDPrivate::Gap);
        const float Y = StartY + Row * (RiftHUDPrivate::SlotSize + RiftHUDPrivate::Gap);
        const FName ItemId = PlayerSlotOrder[Index];
        DrawSlot(X, Y, FName(*FString::Printf(TEXT("P_%d"), Index)), ItemId, GetPlayerItemCount(Player, ItemId), TEXT(""));
    }

    const int32 ContainerCapacity = OpenContainerActor ? FMath::Clamp(OpenContainerActor->CapacitySlots, 1, 30) : 20;
    for (int32 Index = 0; Index < ContainerCapacity; ++Index)
    {
        const int32 Row = Index / RiftHUDPrivate::Columns;
        const int32 Col = Index % RiftHUDPrivate::Columns;
        const float X = StartXRight + Col * (RiftHUDPrivate::SlotSize + RiftHUDPrivate::Gap);
        const float Y = StartY + Row * (RiftHUDPrivate::SlotSize + RiftHUDPrivate::Gap);
        FName ItemId = NAME_None;
        int32 Count = 0;
        FString Friendly;
        if (OpenContainerActor && OpenContainerActor->Items.IsValidIndex(Index))
        {
            const FRiftLootStack& Stack = OpenContainerActor->Items[Index];
            ItemId = Stack.ItemId;
            Count = Stack.Amount;
            Friendly = Stack.DisplayName.ToString();
        }
        DrawSlot(X, Y, FName(*FString::Printf(TEXT("C_%d"), Index)), ItemId, Count, Friendly);
    }

    if (!OpenContainerActor)
    {
        DrawText(TEXT("Open a crate, cabinet or supply box\nto transfer items here."), FLinearColor(0.55f, 0.63f, 0.66f, 1.0f), RightX + 58.0f, TopY + 330.0f, Font, 1.0f, false);
    }

    DrawText(TEXT("DRAG STACKS BETWEEN SLOTS  |  TAB / I CLOSE"), FLinearColor(0.60f, 0.69f, 0.72f, 1.0f), LeftX + 24.0f, TopY + PanelH - 36.0f, Font, 0.82f, false);
    if (!InventoryMessage.IsEmpty())
    {
        DrawText(InventoryMessage.ToString(), Accent, RightX + 24.0f, TopY + PanelH - 36.0f, Font, 0.82f, false);
    }

    AddHitBox(FVector2D(RightX + PanelW - 44.0f, TopY + 12.0f), FVector2D(30.0f, 30.0f), TEXT("INV_CLOSE"), true, 10);
    DrawText(TEXT("X"), Text, RightX + PanelW - 37.0f, TopY + 15.0f, Font, 1.1f, false);

    if (!DragSourceBox.IsNone() && PlayerOwner)
    {
        float MX = 0.0f;
        float MY = 0.0f;
        if (PlayerOwner->GetMousePosition(MX, MY))
        {
            DrawRect(FLinearColor(0.95f, 0.70f, 0.28f, 0.35f), MX - 24.0f, MY - 24.0f, 48.0f, 48.0f);
        }
    }
}

bool ARiftHUD::ParseSlotName(FName BoxName, TCHAR& OutSide, int32& OutIndex) const
{
    const FString Value = BoxName.ToString();
    if (Value.Len() < 3 || Value[1] != TCHAR('_') || (Value[0] != TCHAR('P') && Value[0] != TCHAR('C')))
    {
        return false;
    }
    OutSide = Value[0];
    OutIndex = FCString::Atoi(*Value.Mid(2));
    return OutIndex >= 0;
}

void ARiftHUD::NotifyHitBoxClick(FName BoxName)
{
    Super::NotifyHitBoxClick(BoxName);
    if (!bInventoryVisible)
    {
        return;
    }
    if (BoxName == TEXT("INV_CLOSE"))
    {
        CloseInventory();
        return;
    }
    TCHAR Side = 0;
    int32 Index = INDEX_NONE;
    if (ParseSlotName(BoxName, Side, Index))
    {
        DragSourceBox = BoxName;
        InventoryMessage = FText::GetEmpty();
    }
}

void ARiftHUD::NotifyHitBoxRelease(FName BoxName)
{
    Super::NotifyHitBoxRelease(BoxName);
    if (!bInventoryVisible || DragSourceBox.IsNone())
    {
        return;
    }

    ARiftPlayerCharacter* Player = PlayerOwner ? Cast<ARiftPlayerCharacter>(PlayerOwner->GetPawn()) : nullptr;
    TCHAR SourceSide = 0;
    TCHAR DestSide = 0;
    int32 SourceIndex = INDEX_NONE;
    int32 DestIndex = INDEX_NONE;
    const bool bSourceValid = ParseSlotName(DragSourceBox, SourceSide, SourceIndex);
    const bool bDestValid = ParseSlotName(BoxName, DestSide, DestIndex);

    if (Player && bSourceValid && bDestValid)
    {
        if (SourceSide == TCHAR('P') && DestSide == TCHAR('P'))
        {
            if (PlayerSlotOrder.IsValidIndex(SourceIndex) && PlayerSlotOrder.IsValidIndex(DestIndex))
            {
                PlayerSlotOrder.Swap(SourceIndex, DestIndex);
            }
        }
        else if (SourceSide == TCHAR('C') && DestSide == TCHAR('C') && OpenContainerActor)
        {
            OpenContainerActor->SwapSlots(SourceIndex, DestIndex);
        }
        else if (SourceSide == TCHAR('P') && DestSide == TCHAR('C'))
        {
            TransferPlayerToContainer(Player, SourceIndex, DestIndex);
        }
        else if (SourceSide == TCHAR('C') && DestSide == TCHAR('P'))
        {
            TransferContainerToPlayer(Player, SourceIndex, DestIndex);
        }
    }

    DragSourceBox = NAME_None;
}

bool ARiftHUD::TransferPlayerToContainer(ARiftPlayerCharacter* Player, int32 PlayerSlot, int32 ContainerSlot)
{
    if (!Player || !OpenContainerActor || !PlayerSlotOrder.IsValidIndex(PlayerSlot))
    {
        return false;
    }
    const FName ItemId = PlayerSlotOrder[PlayerSlot];
    const int32 Amount = GetPlayerItemCount(Player, ItemId);
    if (ItemId.IsNone() || Amount <= 0)
    {
        return false;
    }

    if (!OpenContainerActor->AddItem(ItemId, Amount, FText::FromString(RiftHUDPrivate::FriendlyItemName(ItemId))))
    {
        InventoryMessage = FText::FromString(TEXT("Container has no free slot"));
        return false;
    }

    bool bConsumed = false;
    if (ItemId == TEXT("scrap"))
    {
        Player->Scrap -= Amount;
        bConsumed = true;
    }
    else
    {
        bConsumed = Player->ConsumeComponentItem(ItemId, Amount);
    }

    if (bConsumed)
    {
        PlayerSlotOrder[PlayerSlot] = NAME_None;
        Player->BP_OnInventoryChanged();
        return true;
    }
    return false;
}

bool ARiftHUD::TransferContainerToPlayer(ARiftPlayerCharacter* Player, int32 ContainerSlot, int32 PlayerSlot)
{
    if (!Player || !OpenContainerActor || !OpenContainerActor->Items.IsValidIndex(ContainerSlot))
    {
        return false;
    }

    const FRiftLootStack Stack = OpenContainerActor->Items[ContainerSlot];
    FText Reason;
    if (!URiftInventoryRules::CanAcceptItem(Player, Stack.ItemId, Stack.Amount, Reason))
    {
        InventoryMessage = Reason;
        return false;
    }

    FRiftLootStack Removed;
    if (!OpenContainerActor->RemoveItemAt(ContainerSlot, Stack.Amount, Removed))
    {
        return false;
    }

    if (Removed.ItemId == TEXT("scrap"))
    {
        Player->Scrap += Removed.Amount;
        Player->BP_OnInventoryChanged();
    }
    else
    {
        Player->AddComponentItem(Removed.ItemId, Removed.Amount);
    }

    RefreshPlayerSlotOrder(Player);
    if (PlayerSlotOrder.IsValidIndex(PlayerSlot))
    {
        const int32 CurrentIndex = PlayerSlotOrder.IndexOfByKey(Removed.ItemId);
        if (CurrentIndex != INDEX_NONE && CurrentIndex != PlayerSlot)
        {
            PlayerSlotOrder.Swap(CurrentIndex, PlayerSlot);
        }
    }
    return true;
}

void ARiftHUD::SetInventoryInputMode(bool bEnabled)
{
    if (!PlayerOwner)
    {
        return;
    }

    PlayerOwner->bShowMouseCursor = bEnabled;
    PlayerOwner->bEnableClickEvents = bEnabled;
    PlayerOwner->bEnableMouseOverEvents = bEnabled;
    PlayerOwner->SetIgnoreMoveInput(bEnabled);
    PlayerOwner->SetIgnoreLookInput(bEnabled);

    if (bEnabled)
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        PlayerOwner->SetInputMode(Mode);
    }
    else
    {
        FInputModeGameOnly Mode;
        PlayerOwner->SetInputMode(Mode);
    }
}

void ARiftHUD::ToggleInventory()
{
    if (bInventoryVisible)
    {
        CloseInventory();
    }
    else
    {
        bInventoryVisible = true;
        OpenContainerActor = nullptr;
        InventoryMessage = FText::GetEmpty();
        SetInventoryInputMode(true);
    }
}

void ARiftHUD::OpenContainer(ARiftLootContainer* Container)
{
    if (OpenContainerActor && OpenContainerActor != Container)
    {
        OpenContainerActor->SetContainerOpen(false);
    }
    OpenContainerActor = Container;
    bInventoryVisible = true;
    InventoryMessage = FText::GetEmpty();
    SetInventoryInputMode(true);
}

void ARiftHUD::CloseInventory()
{
    if (OpenContainerActor)
    {
        OpenContainerActor->SetContainerOpen(false);
    }
    OpenContainerActor = nullptr;
    bInventoryVisible = false;
    DragSourceBox = NAME_None;
    InventoryMessage = FText::GetEmpty();
    SetInventoryInputMode(false);
}
