#include "RiftRoadGraph.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ARiftRoadGraph::ARiftRoadGraph()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    RoadInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RoadInstances"));
    RoadInstances->SetupAttachment(Root);
    RoadInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    CurbInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CurbInstances"));
    CurbInstances->SetupAttachment(Root);
    CurbInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        RoadInstances->SetStaticMesh(Cube.Object);
        CurbInstances->SetStaticMesh(Cube.Object);
    }
}

void ARiftRoadGraph::BeginPlay()
{
    Super::BeginPlay();
    GenerateRoadGraph();
}

void ARiftRoadGraph::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    GenerateRoadGraph();
}

void ARiftRoadGraph::GenerateRoadGraph()
{
    if (!RoadInstances || !CurbInstances)
    {
        return;
    }

    RoadInstances->ClearInstances();
    CurbInstances->ClearInstances();
    GeneratedNodes.Reset();
    GeneratedEdges.Reset();

    const int32 Extent = FMath::Max(2, GridHalfExtent);
    const float Cell = FMath::Max(800.0f, CellSize);
    FRandomStream Stream(Seed);

    // Two guaranteed arterial spines ensure that every generated branch ultimately connects to the network.
    for (int32 X = -Extent; X < Extent; ++X)
    {
        AddEdge(FVector(X * Cell, 0.0f, 0.0f), FVector((X + 1) * Cell, 0.0f, 0.0f), true);
    }
    for (int32 Y = -Extent; Y < Extent; ++Y)
    {
        AddEdge(FVector(0.0f, Y * Cell, 0.0f), FVector(0.0f, (Y + 1) * Cell, 0.0f), true);
    }

    // Secondary branches grow out of the arterial spines. Because each begins on an arterial,
    // disconnected decorative road islands are impossible.
    for (int32 X = -Extent + 1; X <= Extent - 1; ++X)
    {
        if (X == 0 || Stream.FRand() > BranchChance)
        {
            continue;
        }

        const int32 Direction = Stream.FRand() < 0.5f ? -1 : 1;
        const int32 LengthCells = Stream.RandRange(1, Extent);
        FVector Current(X * Cell, 0.0f, 0.0f);
        for (int32 Step = 0; Step < LengthCells; ++Step)
        {
            const FVector Next(X * Cell, (Step + 1) * Direction * Cell, 0.0f);
            AddEdge(Current, Next, false);
            Current = Next;

            // Occasional short cross street adds local loops without turning the graph into a perfect grid.
            if (Step > 0 && Stream.FRand() < 0.30f)
            {
                const int32 Side = Stream.FRand() < 0.5f ? -1 : 1;
                AddEdge(Current, Current + FVector(Side * Cell, 0.0f, 0.0f), false);
            }
        }
    }

    for (int32 Y = -Extent + 1; Y <= Extent - 1; ++Y)
    {
        if (Y == 0 || Stream.FRand() > BranchChance * 0.78f)
        {
            continue;
        }

        const int32 Direction = Stream.FRand() < 0.5f ? -1 : 1;
        const int32 LengthCells = Stream.RandRange(1, FMath::Max(1, Extent - 1));
        FVector Current(0.0f, Y * Cell, 0.0f);
        for (int32 Step = 0; Step < LengthCells; ++Step)
        {
            const FVector Next((Step + 1) * Direction * Cell, Y * Cell, 0.0f);
            AddEdge(Current, Next, false);
            Current = Next;
        }
    }

    // Ring-road fragments create recognizable outskirts and longer driving lines.
    if (Extent >= 3)
    {
        const float Ring = Extent * Cell;
        for (int32 I = -Extent; I < Extent; ++I)
        {
            if (Stream.FRand() < 0.70f)
            {
                AddEdge(FVector(I * Cell, Ring, 0.0f), FVector((I + 1) * Cell, Ring, 0.0f), false);
            }
            if (Stream.FRand() < 0.70f)
            {
                AddEdge(FVector(I * Cell, -Ring, 0.0f), FVector((I + 1) * Cell, -Ring, 0.0f), false);
            }
        }
    }

    for (const FRiftRoadEdge& Edge : GeneratedEdges)
    {
        AddRoadGeometry(Edge);
    }
}

void ARiftRoadGraph::AddEdge(const FVector& Start, const FVector& End, bool bPrimary)
{
    if (FVector::DistSquared2D(Start, End) < 100.0f)
    {
        return;
    }

    // Avoid exact duplicates regardless of direction.
    for (const FRiftRoadEdge& Existing : GeneratedEdges)
    {
        const bool bSameDirection = Existing.Start.Equals(Start, 1.0f) && Existing.End.Equals(End, 1.0f);
        const bool bReverseDirection = Existing.Start.Equals(End, 1.0f) && Existing.End.Equals(Start, 1.0f);
        if (bSameDirection || bReverseDirection)
        {
            return;
        }
    }

    FRiftRoadEdge Edge;
    Edge.Start = Start;
    Edge.End = End;
    Edge.bPrimary = bPrimary;
    GeneratedEdges.Add(Edge);
    AddUniqueNode(Start);
    AddUniqueNode(End);
}

void ARiftRoadGraph::AddUniqueNode(const FVector& Node)
{
    for (const FVector& Existing : GeneratedNodes)
    {
        if (Existing.Equals(Node, 1.0f))
        {
            return;
        }
    }
    GeneratedNodes.Add(Node);
}

void ARiftRoadGraph::AddRoadGeometry(const FRiftRoadEdge& Edge)
{
    const FVector Delta = Edge.End - Edge.Start;
    const float Length = Delta.Size2D();
    if (Length <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FVector Direction = Delta.GetSafeNormal2D();
    const FVector Right(-Direction.Y, Direction.X, 0.0f);
    const FRotator Rotation(0.0f, Direction.Rotation().Yaw, 0.0f);
    const float EffectiveSegmentLength = FMath::Clamp(SegmentLength, 120.0f, 900.0f);
    const int32 Segments = FMath::Max(1, FMath::CeilToInt(Length / EffectiveSegmentLength));
    const float ActualLength = Length / Segments;
    const float Width = RoadWidth * (Edge.bPrimary ? 1.14f : 1.0f);

    for (int32 Index = 0; Index < Segments; ++Index)
    {
        const float Distance = (Index + 0.5f) * ActualLength;
        const FVector Center = Edge.Start + Direction * Distance;
        RoadInstances->AddInstance(FTransform(
            Rotation,
            Center + FVector(0.0f, 0.0f, -3.0f),
            FVector(ActualLength / 100.0f, Width / 100.0f, 0.08f)));

        const float CurbOffset = Width * 0.5f + 24.0f;
        const FVector CurbScale(ActualLength / 100.0f, 0.22f, 0.18f);
        CurbInstances->AddInstance(FTransform(Rotation, Center + Right * CurbOffset + FVector(0.0f, 0.0f, 9.0f), CurbScale));
        CurbInstances->AddInstance(FTransform(Rotation, Center - Right * CurbOffset + FVector(0.0f, 0.0f, 9.0f), CurbScale));
    }
}

FVector ARiftRoadGraph::GetNearestRoadNode(FVector WorldLocation) const
{
    if (GeneratedNodes.Num() == 0)
    {
        return GetActorLocation();
    }

    const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
    FVector Best = GeneratedNodes[0];
    float BestDistance = FVector::DistSquared2D(Local, Best);
    for (int32 Index = 1; Index < GeneratedNodes.Num(); ++Index)
    {
        const float Distance = FVector::DistSquared2D(Local, GeneratedNodes[Index]);
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            Best = GeneratedNodes[Index];
        }
    }
    return GetActorTransform().TransformPosition(Best);
}
