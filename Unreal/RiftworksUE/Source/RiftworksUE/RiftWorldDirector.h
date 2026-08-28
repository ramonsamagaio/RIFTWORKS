#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiftWorldDirector.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;

UCLASS()
class RIFTWORKSUE_API ARiftWorldChunk : public AActor
{
    GENERATED_BODY()

public:
    ARiftWorldChunk();

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GroundInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BuildingInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RoadInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TrunkInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FoliageInstances;

    void BuildSurfaceChunk(FIntPoint Key, int32 WorldSeed, float ChunkSize);
};

UCLASS(Blueprintable)
class RIFTWORKSUE_API ARiftWorldDirector : public AActor
{
    GENERATED_BODY()

public:
    ARiftWorldDirector();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|World")
    int32 WorldSeed = 731942;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|World")
    float ChunkSize = 6400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|World")
    int32 ActiveRadius = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|World")
    float CoreRadius = 11200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|World")
    float ChunkRefreshSeconds = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RIFTWORKS|World")
    int32 MaxChunksPerRefresh = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Lighting")
    TObjectPtr<UDirectionalLightComponent> MoonLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Lighting")
    TObjectPtr<USkyLightComponent> NightSkyLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Lighting")
    TObjectPtr<UExponentialHeightFogComponent> AtmosphereFog;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RIFTWORKS|Lighting")
    TObjectPtr<UPostProcessComponent> NightPostProcess;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="RIFTWORKS|World")
    void RegenerateVisibleWorld();

    UFUNCTION(BlueprintImplementableEvent, Category="RIFTWORKS|Blueprint Events")
    void BP_OnChunkGenerated(FIntPoint ChunkKey, ARiftWorldChunk* Chunk);

protected:
    virtual void BeginPlay() override;

    UPROPERTY()
    TMap<FIntPoint, TObjectPtr<ARiftWorldChunk>> ActiveChunks;

    TArray<FIntPoint> PendingChunks;
    FIntPoint LastCenter = FIntPoint(MAX_int32, MAX_int32);
    float RefreshTimer = 0.0f;

    void RefreshStreaming();
    void ProcessChunkBudget();
    void GenerateUndergroundPrototype();
    FIntPoint PlayerChunk() const;
};
