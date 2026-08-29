#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiftRoadGraph.generated.h"

class USceneComponent;
class UHierarchicalInstancedStaticMeshComponent;

USTRUCT(BlueprintType)
struct FRiftRoadEdge
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Roads")
    FVector Start = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Roads")
    FVector End = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="RIFTWORKS|Roads")
    bool bPrimary = false;
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftRoadGraph : public AActor
{
    GENERATED_BODY()

public:
    ARiftRoadGraph();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Roads")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Roads")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RoadInstances;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Roads")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CurbInstances;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Roads")
    int32 Seed = 731942;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Roads", meta=(ClampMin="2", ClampMax="12"))
    int32 GridHalfExtent = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Roads")
    float CellSize = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Roads")
    float RoadWidth = 760.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Roads")
    float SegmentLength = 380.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|Roads", meta=(ClampMin="0.0", ClampMax="1.0"))
    float BranchChance = 0.62f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Roads")
    TArray<FVector> GeneratedNodes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Roads")
    TArray<FRiftRoadEdge> GeneratedEdges;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="RIFTWORKS|Roads")
    void GenerateRoadGraph();

    UFUNCTION(BlueprintPure, Category="RIFTWORKS|Roads")
    FVector GetNearestRoadNode(FVector WorldLocation) const;

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

    void AddEdge(const FVector& Start, const FVector& End, bool bPrimary);
    void AddRoadGeometry(const FRiftRoadEdge& Edge);
    void AddUniqueNode(const FVector& Node);
};
