#include "RiftLootContainer.h"

#include "RiftHUD.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

namespace RiftLootPrivate
{
    UMaterialInterface* LoadMaterial(const TCHAR* Path)
    {
        return Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, Path));
    }
}

ARiftLootContainer::ARiftLootContainer()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Root);
    Body->SetStaticMesh(Cube.Object);
    Body->SetRelativeScale3D(FVector(1.15f, 0.72f, 0.48f));
    Body->SetRelativeLocation(FVector(0.0f, 0.0f, 48.0f));
    Body->SetCollisionProfileName(TEXT("BlockAll"));

    Lid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lid"));
    Lid->SetupAttachment(Root);
    Lid->SetStaticMesh(Cube.Object);
    Lid->SetRelativeScale3D(FVector(1.20f, 0.76f, 0.10f));
    Lid->SetRelativeLocation(FVector(0.0f, 0.0f, 101.0f));
    Lid->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BandA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BandA"));
    BandA->SetupAttachment(Root);
    BandA->SetStaticMesh(Cube.Object);
    BandA->SetRelativeScale3D(FVector(0.10f, 0.77f, 0.52f));
    BandA->SetRelativeLocation(FVector(-72.0f, 0.0f, 49.0f));
    BandA->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BandB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BandB"));
    BandB->SetupAttachment(Root);
    BandB->SetStaticMesh(Cube.Object);
    BandB->SetRelativeScale3D(FVector(0.10f, 0.77f, 0.52f));
    BandB->SetRelativeLocation(FVector(72.0f, 0.0f, 49.0f));
    BandB->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Handle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Handle"));
    Handle->SetupAttachment(Root);
    Handle->SetStaticMesh(Cube.Object);
    Handle->SetRelativeScale3D(FVector(0.28f, 0.08f, 0.08f));
    Handle->SetRelativeLocation(FVector(0.0f, -77.0f, 68.0f));
    Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARiftLootContainer::BeginPlay()
{
    Super::BeginPlay();
    ApplyRuntimeMaterials();
    if (bSeedDefaultLoot && Items.Num() == 0)
    {
        SeedLoot();
    }
}

void ARiftLootContainer::ApplyRuntimeMaterials()
{
    UMaterialInterface* BodyMat = RiftLootPrivate::LoadMaterial(TEXT("/Game/Riftworks/Materials/World/M_Metal_Industrial.M_Metal_Industrial"));
    UMaterialInterface* BandMat = RiftLootPrivate::LoadMaterial(TEXT("/Game/Riftworks/Materials/World/M_Assembly_Motor.M_Assembly_Motor"));
    if (BodyMat)
    {
        Body->SetMaterial(0, BodyMat);
        Lid->SetMaterial(0, BodyMat);
    }
    if (BandMat)
    {
        BandA->SetMaterial(0, BandMat);
        BandB->SetMaterial(0, BandMat);
        Handle->SetMaterial(0, BandMat);
    }
}

FText ARiftLootContainer::GetInteractionText_Implementation() const
{
    return FText::FromString(FString::Printf(TEXT("E  OPEN %s"), *ContainerName.ToString().ToUpper()));
}

void ARiftLootContainer::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    if (!Player)
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(Player->GetController());
    ARiftHUD* HUD = PC ? Cast<ARiftHUD>(PC->GetHUD()) : nullptr;
    if (HUD)
    {
        SetContainerOpen(true);
        HUD->OpenContainer(this);
    }
}

void ARiftLootContainer::SetContainerOpen(bool bOpen)
{
    bOpenVisual = bOpen;
    if (Lid)
    {
        Lid->SetRelativeLocation(bOpen ? FVector(0.0f, 32.0f, 135.0f) : FVector(0.0f, 0.0f, 101.0f));
        Lid->SetRelativeRotation(bOpen ? FRotator(-28.0f, 0.0f, 0.0f) : FRotator::ZeroRotator);
    }
}

void ARiftLootContainer::AddSeeded(FRandomStream& Stream, FName ItemId, const TCHAR* Label, int32 MinAmount, int32 MaxAmount)
{
    AddItem(ItemId, Stream.RandRange(MinAmount, MaxAmount), FText::FromString(Label));
}

void ARiftLootContainer::SeedLoot()
{
    Items.Reset();
    FRandomStream Stream(LootSeed != 0 ? LootSeed : GetUniqueID());

    AddSeeded(Stream, TEXT("scrap"), TEXT("Machined Scrap"), 3 + LootTier, 7 + LootTier * 2);
    AddSeeded(Stream, TEXT("fasteners"), TEXT("Fasteners"), 2, 5 + LootTier);

    const int32 RollCount = FMath::Clamp(2 + LootTier, 3, 6);
    const TArray<FName> Pool = {
        TEXT("cable"), TEXT("electronics"), TEXT("battery"), TEXT("medical_supplies"),
        TEXT("copper_coil"), TEXT("fuel"), TEXT("replacement_part"), TEXT("ammo")
    };
    const TMap<FName, FString> Names = {
        {TEXT("cable"), TEXT("Cable Bundle")},
        {TEXT("electronics"), TEXT("Electronics")},
        {TEXT("battery"), TEXT("Battery Pack")},
        {TEXT("medical_supplies"), TEXT("Medical Supplies")},
        {TEXT("copper_coil"), TEXT("Copper Coil")},
        {TEXT("fuel"), TEXT("Fuel Canister")},
        {TEXT("replacement_part"), TEXT("Replacement Part")},
        {TEXT("ammo"), TEXT("Rifle Ammunition")}
    };

    for (int32 Index = 0; Index < RollCount; ++Index)
    {
        const FName ItemId = Pool[Stream.RandRange(0, Pool.Num() - 1)];
        const FString* Friendly = Names.Find(ItemId);
        AddItem(ItemId, Stream.RandRange(1, FMath::Max(2, LootTier + 1)), FText::FromString(Friendly ? *Friendly : ItemId.ToString()));
    }

    if (LootTier >= 3 && Stream.FRand() > 0.45f)
    {
        AddItem(TEXT("motor"), 1, FText::FromString(TEXT("Industrial Motor")));
    }
    if (LootTier >= 4 && Stream.FRand() > 0.60f)
    {
        AddItem(TEXT("breach_core"), 1, FText::FromString(TEXT("Breach Core")));
    }
}

bool ARiftLootContainer::AddItem(FName ItemId, int32 Amount, const FText& DisplayName)
{
    if (ItemId.IsNone() || Amount <= 0)
    {
        return false;
    }

    for (FRiftLootStack& Stack : Items)
    {
        if (Stack.ItemId == ItemId)
        {
            Stack.Amount += Amount;
            return true;
        }
    }

    if (Items.Num() >= CapacitySlots)
    {
        return false;
    }

    FRiftLootStack Stack;
    Stack.ItemId = ItemId;
    Stack.DisplayName = DisplayName.IsEmpty() ? FText::FromName(ItemId) : DisplayName;
    Stack.Amount = Amount;
    Items.Add(Stack);
    return true;
}

bool ARiftLootContainer::RemoveItemAt(int32 SlotIndex, int32 Amount, FRiftLootStack& OutRemoved)
{
    if (!Items.IsValidIndex(SlotIndex) || Amount <= 0)
    {
        return false;
    }

    FRiftLootStack& Stack = Items[SlotIndex];
    const int32 RemovedAmount = FMath::Min(Amount, Stack.Amount);
    if (RemovedAmount <= 0)
    {
        return false;
    }

    OutRemoved = Stack;
    OutRemoved.Amount = RemovedAmount;
    Stack.Amount -= RemovedAmount;
    if (Stack.Amount <= 0)
    {
        Items.RemoveAt(SlotIndex);
    }
    return true;
}

bool ARiftLootContainer::SwapSlots(int32 SlotA, int32 SlotB)
{
    if (!Items.IsValidIndex(SlotA) || !Items.IsValidIndex(SlotB) || SlotA == SlotB)
    {
        return false;
    }
    Items.Swap(SlotA, SlotB);
    return true;
}
