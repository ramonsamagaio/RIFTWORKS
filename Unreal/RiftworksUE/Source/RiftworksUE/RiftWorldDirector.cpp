#include "RiftWorldDirector.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace RiftWorldPrivate
{
    void AddBox(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Location, const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator)
    {
        if (!Component)
        {
            return;
        }
        Component->AddInstance(FTransform(Rotation, Location, FVector(Size.X / 100.0f, Size.Y / 100.0f, Size.Z / 100.0f)));
    }

    void AddDoorWall(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Center, float Width, float Height, float Thickness, float DoorWidth, bool bAlongX)
    {
        const float Segment = FMath::Max(80.0f, (Width - DoorWidth) * 0.5f);
        if (bAlongX)
        {
            AddBox(Component, Center + FVector(-(DoorWidth + Segment) * 0.5f, 0.0f, Height * 0.5f), FVector(Segment, Thickness, Height));
            AddBox(Component, Center + FVector((DoorWidth + Segment) * 0.5f, 0.0f, Height * 0.5f), FVector(Segment, Thickness, Height));
            AddBox(Component, Center + FVector(0.0f, 0.0f, Height - 35.0f), FVector(DoorWidth, Thickness, 70.0f));
        }
        else
        {
            AddBox(Component, Center + FVector(0.0f, -(DoorWidth + Segment) * 0.5f, Height * 0.5f), FVector(Thickness, Segment, Height));
            AddBox(Component, Center + FVector(0.0f, (DoorWidth + Segment) * 0.5f, Height * 0.5f), FVector(Thickness, Segment, Height));
            AddBox(Component, Center + FVector(0.0f, 0.0f, Height - 35.0f), FVector(Thickness, DoorWidth, 70.0f));
        }
    }

    void AddOpenBuilding(UHierarchicalInstancedStaticMeshComponent* Building, UHierarchicalInstancedStaticMeshComponent* Floor,
        const FVector& Center, float Width, float Depth, float Height, bool bIndustrial, FRandomStream& Stream)
    {
        const float Wall = bIndustrial ? 28.0f : 22.0f;
        const float Door = bIndustrial ? FMath::Min(520.0f, Width * 0.48f) : 180.0f;
        const float FloorThickness = 24.0f;

        AddBox(Floor, Center + FVector(0.0f, 0.0f, FloorThickness * 0.5f), FVector(Width, Depth, FloorThickness));
        AddBox(Building, Center + FVector(0.0f, Depth * 0.5f, Height * 0.5f), FVector(Width, Wall, Height));
        AddBox(Building, Center + FVector(-Width * 0.5f, 0.0f, Height * 0.5f), FVector(Wall, Depth, Height));
        AddBox(Building, Center + FVector(Width * 0.5f, 0.0f, Height * 0.5f), FVector(Wall, Depth, Height));
        AddDoorWall(Building, Center + FVector(0.0f, -Depth * 0.5f, 0.0f), Width, Height, Wall, Door, true);
        AddBox(Building, Center + FVector(0.0f, 0.0f, Height + 14.0f), FVector(Width + 45.0f, Depth + 45.0f, 28.0f));

        // Architectural rhythm instead of featureless cubes.
        const int32 BayCount = bIndustrial ? 4 : 3;
        for (int32 Bay = 1; Bay < BayCount; ++Bay)
        {
            const float Alpha = static_cast<float>(Bay) / static_cast<float>(BayCount);
            const float LocalX = FMath::Lerp(-Width * 0.5f, Width * 0.5f, Alpha);
            AddBox(Building, Center + FVector(LocalX, Depth * 0.5f + 16.0f, Height * 0.52f), FVector(18.0f, 32.0f, Height * 0.82f));
        }

        if (bIndustrial)
        {
            // Loading canopy + rooftop machinery silhouette.
            AddBox(Building, Center + FVector(0.0f, -Depth * 0.5f - 150.0f, Height * 0.72f), FVector(FMath::Min(Width * 0.72f, 950.0f), 300.0f, 24.0f));
            AddBox(Building, Center + FVector(-Width * 0.24f, -Depth * 0.5f - 150.0f, Height * 0.35f), FVector(26.0f, 26.0f, Height * 0.7f));
            AddBox(Building, Center + FVector(Width * 0.24f, -Depth * 0.5f - 150.0f, Height * 0.35f), FVector(26.0f, 26.0f, Height * 0.7f));
            AddBox(Building, Center + FVector(Width * 0.22f, Depth * 0.08f, Height + 95.0f), FVector(260.0f, 220.0f, 150.0f));
        }
        else
        {
            // Porch / sign / rooftop block gives residential-commercial silhouettes more character.
            AddBox(Building, Center + FVector(0.0f, -Depth * 0.5f - 75.0f, 230.0f), FVector(FMath::Min(Width * 0.58f, 520.0f), 150.0f, 18.0f));
            if (Stream.FRand() < 0.55f)
            {
                AddBox(Building, Center + FVector(Width * 0.26f, 0.0f, Height + 55.0f), FVector(170.0f, 140.0f, 90.0f));
            }
        }
    }

    void AddTunnelSection(ARiftWorldChunk* Chunk, const FVector& Center, float Width, float Length, float Height)
    {
        if (!Chunk)
        {
            return;
        }
        AddBox(Chunk->RoadInstances, Center + FVector(0.0f, 0.0f, -16.0f), FVector(Width, Length, 32.0f));
        AddBox(Chunk->BuildingInstances, Center + FVector(-Width * 0.5f, 0.0f, Height * 0.5f), FVector(30.0f, Length, Height));
        AddBox(Chunk->BuildingInstances, Center + FVector(Width * 0.5f, 0.0f, Height * 0.5f), FVector(30.0f, Length, Height));
        AddBox(Chunk->BuildingInstances, Center + FVector(0.0f, 0.0f, Height), FVector(Width, Length, 26.0f));

        for (float Y = -Length * 0.4f; Y <= Length * 0.4f; Y += 420.0f)
        {
            AddBox(Chunk->BuildingInstances, Center + FVector(-Width * 0.44f, Y, Height * 0.5f), FVector(34.0f, 34.0f, Height));
            AddBox(Chunk->BuildingInstances, Center + FVector(Width * 0.44f, Y, Height * 0.5f), FVector(34.0f, 34.0f, Height));
            AddBox(Chunk->BuildingInstances, Center + FVector(0.0f, Y, Height * 0.88f), FVector(Width * 0.82f, 30.0f, 30.0f));
        }
    }
}

ARiftWorldChunk::ARiftWorldChunk()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    GroundInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Ground"));
    BuildingInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Buildings"));
    RoadInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RoadsAndFloors"));
    TrunkInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TreeTrunks"));
    FoliageInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TreeCrowns"));

    GroundInstances->SetupAttachment(Root);
    BuildingInstances->SetupAttachment(Root);
    RoadInstances->SetupAttachment(Root);
    TrunkInstances->SetupAttachment(Root);
    FoliageInstances->SetupAttachment(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));

    if (Cube.Succeeded())
    {
        GroundInstances->SetStaticMesh(Cube.Object);
        BuildingInstances->SetStaticMesh(Cube.Object);
        RoadInstances->SetStaticMesh(Cube.Object);
    }
    if (Cylinder.Succeeded())
    {
        TrunkInstances->SetStaticMesh(Cylinder.Object);
    }
    if (Cone.Succeeded())
    {
        FoliageInstances->SetStaticMesh(Cone.Object);
    }

    GroundInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    BuildingInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    RoadInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    TrunkInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    FoliageInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARiftWorldChunk::BuildSurfaceChunk(FIntPoint Key, int32 WorldSeed, float ChunkSize)
{
    GroundInstances->ClearInstances();
    BuildingInstances->ClearInstances();
    RoadInstances->ClearInstances();
    TrunkInstances->ClearInstances();
    FoliageInstances->ClearInstances();

    const int32 HashedSeed = WorldSeed ^ (Key.X * 92837111) ^ (Key.Y * 689287499);
    FRandomStream Stream(HashedSeed);

    const float RegionNoise = FMath::PerlinNoise2D(FVector2D(
        (Key.X + WorldSeed * 0.001f) * 0.31f,
        (Key.Y - WorldSeed * 0.001f) * 0.31f));
    const float Region = (RegionNoise + 1.0f) * 0.5f;

    RiftWorldPrivate::AddBox(GroundInstances, FVector(ChunkSize * 0.5f, ChunkSize * 0.5f, -50.0f), FVector(ChunkSize, ChunkSize, 100.0f));

    const bool bRoadX = FMath::Abs(Key.Y % 3) == 0;
    const bool bRoadY = FMath::Abs(Key.X % 3) == 0;
    if (bRoadX)
    {
        RiftWorldPrivate::AddBox(RoadInstances, FVector(ChunkSize * 0.5f, ChunkSize * 0.5f, 4.0f), FVector(ChunkSize, 820.0f, 8.0f));
        RiftWorldPrivate::AddBox(BuildingInstances, FVector(ChunkSize * 0.5f, ChunkSize * 0.5f - 500.0f, 12.0f), FVector(ChunkSize, 120.0f, 24.0f));
        RiftWorldPrivate::AddBox(BuildingInstances, FVector(ChunkSize * 0.5f, ChunkSize * 0.5f + 500.0f, 12.0f), FVector(ChunkSize, 120.0f, 24.0f));
    }
    if (bRoadY)
    {
        RiftWorldPrivate::AddBox(RoadInstances, FVector(ChunkSize * 0.5f, ChunkSize * 0.5f, 5.0f), FVector(820.0f, ChunkSize, 10.0f));
        RiftWorldPrivate::AddBox(BuildingInstances, FVector(ChunkSize * 0.5f - 500.0f, ChunkSize * 0.5f, 12.0f), FVector(120.0f, ChunkSize, 24.0f));
        RiftWorldPrivate::AddBox(BuildingInstances, FVector(ChunkSize * 0.5f + 500.0f, ChunkSize * 0.5f, 12.0f), FVector(120.0f, ChunkSize, 24.0f));
    }

    const float UrbanWeight = FMath::Clamp((Region - 0.30f) / 0.35f, 0.0f, 1.0f) * FMath::Clamp((0.82f - Region) / 0.18f, 0.0f, 1.0f);
    const float IndustrialWeight = FMath::Clamp((Region - 0.62f) / 0.25f, 0.0f, 1.0f);
    const float WoodlandWeight = FMath::Clamp((0.55f - Region) / 0.35f, 0.0f, 1.0f);

    const int32 BuildingCount = FMath::RoundToInt(FMath::Lerp(1.0f, 7.0f, FMath::Max(UrbanWeight, IndustrialWeight)));
    for (int32 Index = 0; Index < BuildingCount; ++Index)
    {
        float X = Stream.FRandRange(850.0f, ChunkSize - 850.0f);
        float Y = Stream.FRandRange(850.0f, ChunkSize - 850.0f);
        if (bRoadX && FMath::Abs(Y - ChunkSize * 0.5f) < 1050.0f) { Y += (Y < ChunkSize * 0.5f ? -1250.0f : 1250.0f); }
        if (bRoadY && FMath::Abs(X - ChunkSize * 0.5f) < 1050.0f) { X += (X < ChunkSize * 0.5f ? -1250.0f : 1250.0f); }
        X = FMath::Clamp(X, 650.0f, ChunkSize - 650.0f);
        Y = FMath::Clamp(Y, 650.0f, ChunkSize - 650.0f);

        const bool bIndustrial = Stream.FRand() < IndustrialWeight;
        const float Width = bIndustrial ? Stream.FRandRange(1150.0f, 1900.0f) : Stream.FRandRange(720.0f, 1200.0f);
        const float Depth = bIndustrial ? Stream.FRandRange(1050.0f, 1750.0f) : Stream.FRandRange(700.0f, 1150.0f);
        const float Height = bIndustrial ? Stream.FRandRange(410.0f, 620.0f) : Stream.FRandRange(315.0f, 430.0f);
        RiftWorldPrivate::AddOpenBuilding(BuildingInstances, RoadInstances, FVector(X, Y, 0.0f), Width, Depth, Height, bIndustrial, Stream);
    }

    const int32 TreeCount = FMath::RoundToInt(FMath::Lerp(3.0f, 28.0f, WoodlandWeight));
    for (int32 Index = 0; Index < TreeCount; ++Index)
    {
        const float X = Stream.FRandRange(250.0f, ChunkSize - 250.0f);
        const float Y = Stream.FRandRange(250.0f, ChunkSize - 250.0f);
        if ((bRoadX && FMath::Abs(Y - ChunkSize * 0.5f) < 700.0f) || (bRoadY && FMath::Abs(X - ChunkSize * 0.5f) < 700.0f))
        {
            continue;
        }
        const float Scale = Stream.FRandRange(0.75f, 1.35f);
        TrunkInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Y, 150.0f * Scale), FVector(0.42f * Scale, 0.42f * Scale, 3.0f * Scale)));
        FoliageInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Y, 385.0f * Scale), FVector(1.8f * Scale, 1.8f * Scale, 3.4f * Scale)));
    }
}

ARiftWorldDirector::ARiftWorldDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    USceneComponent* RootComponentObject = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(RootComponentObject);

    MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonLight"));
    MoonLight->SetupAttachment(RootComponentObject);
    MoonLight->SetRelativeRotation(FRotator(-54.0f, -31.0f, 0.0f));
    MoonLight->SetIntensity(0.018f);
    MoonLight->SetLightColor(FLinearColor(0.22f, 0.30f, 0.55f));
    MoonLight->CastShadows = true;
    MoonLight->SetVolumetricScatteringIntensity(0.08f);

    NightSkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("NightSkyLight"));
    NightSkyLight->SetupAttachment(RootComponentObject);
    NightSkyLight->SetIntensity(0.012f);
    NightSkyLight->SetLightColor(FLinearColor(0.035f, 0.05f, 0.09f));
    NightSkyLight->Mobility = EComponentMobility::Movable;

    AtmosphereFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("AtmosphereFog"));
    AtmosphereFog->SetupAttachment(RootComponentObject);
    AtmosphereFog->SetFogDensity(0.0035f);
    AtmosphereFog->SetFogHeightFalloff(0.18f);
    AtmosphereFog->SetFogInscatteringColor(FLinearColor(0.012f, 0.018f, 0.035f));
    AtmosphereFog->bEnableVolumetricFog = true;
    AtmosphereFog->VolumetricFogScatteringDistribution = 0.35f;
    AtmosphereFog->VolumetricFogExtinctionScale = 0.28f;
    AtmosphereFog->VolumetricFogAlbedo = FColor(120, 135, 160);
    AtmosphereFog->VolumetricFogDistance = 5200.0f;

    NightPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("NightPostProcess"));
    NightPostProcess->SetupAttachment(RootComponentObject);
    NightPostProcess->bUnbound = true;
    NightPostProcess->Settings.bOverride_VignetteIntensity = true;
    NightPostProcess->Settings.VignetteIntensity = 0.10f;
    NightPostProcess->Settings.bOverride_BloomIntensity = true;
    NightPostProcess->Settings.BloomIntensity = 0.08f;
}

void ARiftWorldDirector::BeginPlay()
{
    Super::BeginPlay();
    GenerateUndergroundPrototype();
    RefreshStreaming();
}

void ARiftWorldDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RefreshTimer -= DeltaSeconds;
    if (RefreshTimer <= 0.0f)
    {
        RefreshTimer = ChunkRefreshSeconds;
        RefreshStreaming();
    }
    ProcessChunkBudget();
}

FIntPoint ARiftWorldDirector::PlayerChunk() const
{
    const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Pawn)
    {
        return FIntPoint::ZeroValue;
    }
    const FVector Location = Pawn->GetActorLocation();
    return FIntPoint(FMath::FloorToInt(Location.X / ChunkSize), FMath::FloorToInt(Location.Y / ChunkSize));
}

void ARiftWorldDirector::RefreshStreaming()
{
    const FIntPoint Center = PlayerChunk();
    TSet<FIntPoint> Wanted;
    PendingChunks.Reset();

    for (int32 X = Center.X - ActiveRadius; X <= Center.X + ActiveRadius; ++X)
    {
        for (int32 Y = Center.Y - ActiveRadius; Y <= Center.Y + ActiveRadius; ++Y)
        {
            const FIntPoint Key(X, Y);
            Wanted.Add(Key);
            if (!ActiveChunks.Contains(Key))
            {
                PendingChunks.Add(Key);
            }
        }
    }

    PendingChunks.Sort([Center](const FIntPoint& A, const FIntPoint& B)
    {
        const int32 AD = FMath::Abs(A.X - Center.X) + FMath::Abs(A.Y - Center.Y);
        const int32 BD = FMath::Abs(B.X - Center.X) + FMath::Abs(B.Y - Center.Y);
        return AD < BD;
    });

    TArray<FIntPoint> ExistingKeys;
    ActiveChunks.GetKeys(ExistingKeys);
    for (const FIntPoint& Key : ExistingKeys)
    {
        if (!Wanted.Contains(Key))
        {
            if (ARiftWorldChunk* Chunk = ActiveChunks.FindRef(Key))
            {
                Chunk->Destroy();
            }
            ActiveChunks.Remove(Key);
        }
    }
    LastCenter = Center;
}

void ARiftWorldDirector::ProcessChunkBudget()
{
    int32 Built = 0;
    while (PendingChunks.Num() > 0 && Built < MaxChunksPerRefresh)
    {
        const FIntPoint Key = PendingChunks[0];
        PendingChunks.RemoveAt(0);
        if (ActiveChunks.Contains(Key))
        {
            continue;
        }

        const FVector SpawnLocation(Key.X * ChunkSize, Key.Y * ChunkSize, 0.0f);
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ARiftWorldChunk* Chunk = GetWorld()->SpawnActor<ARiftWorldChunk>(ARiftWorldChunk::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params);
        if (Chunk)
        {
            Chunk->BuildSurfaceChunk(Key, WorldSeed, ChunkSize);
            ActiveChunks.Add(Key, Chunk);
            BP_OnChunkGenerated(Key, Chunk);
        }
        ++Built;
    }
}

void ARiftWorldDirector::RegenerateVisibleWorld()
{
    for (TPair<FIntPoint, TObjectPtr<ARiftWorldChunk>>& Pair : ActiveChunks)
    {
        if (Pair.Value)
        {
            Pair.Value->Destroy();
        }
    }
    ActiveChunks.Reset();
    PendingChunks.Reset();
    LastCenter = FIntPoint(MAX_int32, MAX_int32);
    RefreshStreaming();
}

void ARiftWorldDirector::GenerateUndergroundPrototype()
{
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ARiftWorldChunk* Underground = GetWorld()->SpawnActor<ARiftWorldChunk>(ARiftWorldChunk::StaticClass(), FVector(-4000.0f, -2000.0f, 0.0f), FRotator::ZeroRotator, Params);
    if (!Underground)
    {
        return;
    }
    Underground->SetActorLabel(TEXT("Riftworks_ProceduralUnderground"));

    // Enclosed descending service tunnel.
    for (int32 Step = 0; Step < 18; ++Step)
    {
        const float Y = -Step * 185.0f;
        const float Z = -55.0f - Step * 58.0f;
        const FVector Center(0.0f, Y, Z);
        RiftWorldPrivate::AddBox(Underground->RoadInstances, Center, FVector(820.0f, 195.0f, 28.0f));
        RiftWorldPrivate::AddBox(Underground->BuildingInstances, Center + FVector(-420.0f, 0.0f, 220.0f), FVector(28.0f, 195.0f, 440.0f));
        RiftWorldPrivate::AddBox(Underground->BuildingInstances, Center + FVector(420.0f, 0.0f, 220.0f), FVector(28.0f, 195.0f, 440.0f));
        RiftWorldPrivate::AddBox(Underground->BuildingInstances, Center + FVector(0.0f, 0.0f, 440.0f), FVector(860.0f, 195.0f, 24.0f));
    }

    // Human infrastructure gradually turns into larger, stranger chambers.
    float CurrentY = -4100.0f;
    float CurrentZ = -1120.0f;
    for (int32 Section = 0; Section < 7; ++Section)
    {
        const float Width = Section < 3 ? 920.0f : 1150.0f;
        const float Height = Section < 3 ? 480.0f : 620.0f;
        RiftWorldPrivate::AddTunnelSection(Underground, FVector(0.0f, CurrentY, CurrentZ), Width, 1350.0f, Height);

        if (Section == 1 || Section == 3 || Section == 5)
        {
            const FVector Chamber(0.0f, CurrentY - 950.0f, CurrentZ);
            RiftWorldPrivate::AddBox(Underground->RoadInstances, Chamber, FVector(2500.0f, 1700.0f, 34.0f));
            RiftWorldPrivate::AddBox(Underground->BuildingInstances, Chamber + FVector(-1250.0f, 0.0f, 360.0f), FVector(34.0f, 1700.0f, 720.0f));
            RiftWorldPrivate::AddBox(Underground->BuildingInstances, Chamber + FVector(1250.0f, 0.0f, 360.0f), FVector(34.0f, 1700.0f, 720.0f));
            RiftWorldPrivate::AddDoorWall(Underground->BuildingInstances, Chamber + FVector(0.0f, -850.0f, 0.0f), 2500.0f, 720.0f, 34.0f, 620.0f, true);
            RiftWorldPrivate::AddBox(Underground->BuildingInstances, Chamber + FVector(0.0f, 850.0f, 360.0f), FVector(2500.0f, 34.0f, 720.0f));
            RiftWorldPrivate::AddBox(Underground->BuildingInstances, Chamber + FVector(0.0f, 0.0f, 720.0f), FVector(2500.0f, 1700.0f, 30.0f));

            // A few columns and abandoned machine pedestals make the chambers readable.
            for (int32 P = -1; P <= 1; P += 2)
            {
                RiftWorldPrivate::AddBox(Underground->BuildingInstances, Chamber + FVector(P * 720.0f, 280.0f, 270.0f), FVector(110.0f, 110.0f, 540.0f));
                RiftWorldPrivate::AddBox(Underground->BuildingInstances, Chamber + FVector(P * 530.0f, -260.0f, 90.0f), FVector(300.0f, 240.0f, 180.0f));
            }
            CurrentY -= 1500.0f;
        }

        CurrentY -= 1400.0f;
        CurrentZ -= Section < 3 ? 85.0f : 125.0f;
    }

    UPointLightComponent* BreachLight = NewObject<UPointLightComponent>(Underground, TEXT("DeepBreachLight"));
    BreachLight->RegisterComponent();
    BreachLight->AttachToComponent(Underground->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    BreachLight->SetRelativeLocation(FVector(0.0f, CurrentY + 400.0f, CurrentZ + 220.0f));
    BreachLight->IntensityUnits = ELightUnits::Lumens;
    BreachLight->SetIntensity(2400.0f);
    BreachLight->SetAttenuationRadius(2200.0f);
    BreachLight->SetLightColor(FLinearColor(0.28f, 0.12f, 0.72f));
    BreachLight->SetVolumetricScatteringIntensity(0.20f);
    BreachLight->CastShadows = true;
}
