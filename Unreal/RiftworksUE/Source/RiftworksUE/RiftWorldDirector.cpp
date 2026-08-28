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

ARiftWorldChunk::ARiftWorldChunk()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    GroundInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Ground"));
    BuildingInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Buildings"));
    RoadInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Roads"));
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

    GroundInstances->AddInstance(FTransform(
        FRotator::ZeroRotator,
        FVector(ChunkSize * 0.5f, ChunkSize * 0.5f, -50.0f),
        FVector(ChunkSize / 100.0f, ChunkSize / 100.0f, 1.0f)));

    const bool bRoadX = FMath::Abs(Key.Y % 3) == 0;
    const bool bRoadY = FMath::Abs(Key.X % 3) == 0;
    if (bRoadX)
    {
        RoadInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(ChunkSize * 0.5f, ChunkSize * 0.5f, 3.0f), FVector(ChunkSize / 100.0f, 8.0f, 0.06f)));
    }
    if (bRoadY)
    {
        RoadInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(ChunkSize * 0.5f, ChunkSize * 0.5f, 4.0f), FVector(8.0f, ChunkSize / 100.0f, 0.06f)));
    }

    const float UrbanWeight = FMath::Clamp((Region - 0.30f) / 0.35f, 0.0f, 1.0f) * FMath::Clamp((0.82f - Region) / 0.18f, 0.0f, 1.0f);
    const float IndustrialWeight = FMath::Clamp((Region - 0.62f) / 0.25f, 0.0f, 1.0f);
    const float WoodlandWeight = FMath::Clamp((0.55f - Region) / 0.35f, 0.0f, 1.0f);

    const int32 BuildingCount = FMath::RoundToInt(FMath::Lerp(1.0f, 8.0f, FMath::Max(UrbanWeight, IndustrialWeight)));
    for (int32 Index = 0; Index < BuildingCount; ++Index)
    {
        float X = Stream.FRandRange(700.0f, ChunkSize - 700.0f);
        float Y = Stream.FRandRange(700.0f, ChunkSize - 700.0f);
        if (bRoadX && FMath::Abs(Y - ChunkSize * 0.5f) < 900.0f) { Y += 1500.0f; }
        if (bRoadY && FMath::Abs(X - ChunkSize * 0.5f) < 900.0f) { X += 1500.0f; }
        X = FMath::Clamp(X, 500.0f, ChunkSize - 500.0f);
        Y = FMath::Clamp(Y, 500.0f, ChunkSize - 500.0f);

        const bool bIndustrial = Stream.FRand() < IndustrialWeight;
        const float Width = bIndustrial ? Stream.FRandRange(1300.0f, 2200.0f) : Stream.FRandRange(700.0f, 1400.0f);
        const float Depth = bIndustrial ? Stream.FRandRange(1100.0f, 2000.0f) : Stream.FRandRange(700.0f, 1400.0f);
        const float Height = bIndustrial ? Stream.FRandRange(500.0f, 1000.0f) : Stream.FRandRange(450.0f, 1800.0f);
        BuildingInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Y, Height * 0.5f), FVector(Width / 100.0f, Depth / 100.0f, Height / 100.0f)));
    }

    const int32 TreeCount = FMath::RoundToInt(FMath::Lerp(2.0f, 30.0f, WoodlandWeight));
    for (int32 Index = 0; Index < TreeCount; ++Index)
    {
        const float X = Stream.FRandRange(250.0f, ChunkSize - 250.0f);
        const float Y = Stream.FRandRange(250.0f, ChunkSize - 250.0f);
        if ((bRoadX && FMath::Abs(Y - ChunkSize * 0.5f) < 650.0f) || (bRoadY && FMath::Abs(X - ChunkSize * 0.5f) < 650.0f))
        {
            continue;
        }
        const float Scale = Stream.FRandRange(0.8f, 1.5f);
        TrunkInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Y, 160.0f * Scale), FVector(0.55f * Scale, 0.55f * Scale, 3.2f * Scale)));
        FoliageInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Y, 420.0f * Scale), FVector(2.2f * Scale, 2.2f * Scale, 4.0f * Scale)));
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
    MoonLight->SetIntensity(0.12f);
    MoonLight->SetLightColor(FLinearColor(0.37f, 0.47f, 0.72f));
    MoonLight->CastShadows = true;
    MoonLight->VolumetricScatteringIntensity = 0.35f;

    NightSkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("NightSkyLight"));
    NightSkyLight->SetupAttachment(RootComponentObject);
    NightSkyLight->SetIntensity(0.08f);
    NightSkyLight->SetLightColor(FLinearColor(0.08f, 0.11f, 0.18f));
    NightSkyLight->Mobility = EComponentMobility::Movable;

    AtmosphereFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("AtmosphereFog"));
    AtmosphereFog->SetupAttachment(RootComponentObject);
    AtmosphereFog->SetFogDensity(0.018f);
    AtmosphereFog->SetFogHeightFalloff(0.11f);
    AtmosphereFog->SetFogInscatteringColor(FLinearColor(0.025f, 0.04f, 0.075f));
    AtmosphereFog->bEnableVolumetricFog = true;
    AtmosphereFog->VolumetricFogScatteringDistribution = 0.62f;
    AtmosphereFog->VolumetricFogExtinctionScale = 1.15f;
    AtmosphereFog->VolumetricFogAlbedo = FColor(170, 185, 210);
    AtmosphereFog->VolumetricFogDistance = 9000.0f;

    NightPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("NightPostProcess"));
    NightPostProcess->SetupAttachment(RootComponentObject);
    NightPostProcess->bUnbound = true;
    NightPostProcess->Settings.bOverride_VignetteIntensity = true;
    NightPostProcess->Settings.VignetteIntensity = 0.18f;
    NightPostProcess->Settings.bOverride_BloomIntensity = true;
    NightPostProcess->Settings.BloomIntensity = 0.35f;
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

    for (int32 Step = 0; Step < 12; ++Step)
    {
        const float Y = -Step * 155.0f;
        const float Z = -45.0f - Step * 72.0f;
        Underground->RoadInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, Y, Z), FVector(7.0f, 1.6f, 0.42f)));
    }

    for (int32 Section = 0; Section < 8; ++Section)
    {
        const float Y = -2100.0f - Section * 1200.0f;
        const float Z = -900.0f - Section * 130.0f;
        Underground->RoadInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, Y, Z), FVector(8.0f, 12.0f, 0.4f)));
        Underground->BuildingInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(-410.0f, Y, Z + 320.0f), FVector(0.45f, 12.0f, 6.5f)));
        Underground->BuildingInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(410.0f, Y, Z + 320.0f), FVector(0.45f, 12.0f, 6.5f)));
        if (Section % 2 == 0)
        {
            Underground->BuildingInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, Y - 380.0f, Z + 120.0f), FVector(3.2f, 1.1f, 2.4f)));
        }
    }

    UPointLightComponent* BreachLight = NewObject<UPointLightComponent>(Underground, TEXT("DeepBreachLight"));
    BreachLight->RegisterComponent();
    BreachLight->AttachToComponent(Underground->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    BreachLight->SetRelativeLocation(FVector(0.0f, -10500.0f, -1900.0f));
    BreachLight->IntensityUnits = ELightUnits::Lumens;
    BreachLight->SetIntensity(9000.0f);
    BreachLight->SetAttenuationRadius(2600.0f);
    BreachLight->SetLightColor(FLinearColor(0.38f, 0.18f, 0.85f));
    BreachLight->VolumetricScatteringIntensity = 3.0f;
    BreachLight->CastShadows = true;
}
