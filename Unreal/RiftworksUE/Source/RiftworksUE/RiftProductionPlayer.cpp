#include "RiftProductionPlayer.h"

#include "RiftGameplayActors.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

ARiftProductionPlayerCharacter::ARiftProductionPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ARiftProductionPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    Stamina = FMath::Clamp(Stamina, 0.0f, MaxStamina);
    if (FirstPersonCamera)
    {
        SmoothedCameraHeight = FirstPersonCamera->GetRelativeLocation().Z;
    }
    LastReportedStamina = Stamina;
    BP_OnStaminaChanged(Stamina, MaxStamina, bSprintExhausted);
    BP_OnEngineeringSelectionChanged(nullptr, SelectedJointMode);
}

void ARiftProductionPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (!PlayerInputComponent)
    {
        return;
    }

    PlayerInputComponent->BindAction(TEXT("ProductionBuildPlace"), IE_Pressed, this, &ARiftProductionPlayerCharacter::ProductionBuildPlacePressed);
    PlayerInputComponent->BindAction(TEXT("EngineeringConnect"), IE_Pressed, this, &ARiftProductionPlayerCharacter::EngineeringConnectPressed);
    PlayerInputComponent->BindAction(TEXT("EngineeringModeNext"), IE_Pressed, this, &ARiftProductionPlayerCharacter::EngineeringModeNextPressed);
    PlayerInputComponent->BindAction(TEXT("EngineeringCancel"), IE_Pressed, this, &ARiftProductionPlayerCharacter::EngineeringCancelPressed);
}

void ARiftProductionPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateStamina(DeltaSeconds);
    UpdateFirstPersonCameraMotion(DeltaSeconds);
    UpdateBuildCostPrompt();
    UpdateEngineeringPrompt();
}

void ARiftProductionPlayerCharacter::UpdateStamina(float DeltaSeconds)
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (!Movement)
    {
        return;
    }

    const float Speed = GetVelocity().Size2D();
    const bool bMovementIsSprintSpeed = Movement->MaxWalkSpeed > WalkSpeed + 5.0f;
    const bool bCanActuallySprint = !bIsCrouched && !CarriedSalvage && !bSprintExhausted;
    const bool bActivelySprinting = bMovementIsSprintSpeed && bCanActuallySprint && Speed > WalkSpeed * 0.55f;

    if (bActivelySprinting)
    {
        Stamina = FMath::Max(0.0f, Stamina - SprintDrainPerSecond * DeltaSeconds);
        if (Stamina <= KINDA_SMALL_NUMBER)
        {
            Stamina = 0.0f;
            bSprintExhausted = true;
            Movement->MaxWalkSpeed = WalkSpeed;
        }
    }
    else
    {
        const float RecoveryMultiplier = Speed < 20.0f ? 1.35f : 1.0f;
        Stamina = FMath::Min(MaxStamina, Stamina + StaminaRecoveryPerSecond * RecoveryMultiplier * DeltaSeconds);
        if (bSprintExhausted && Stamina >= SprintRecoveryThreshold)
        {
            bSprintExhausted = false;
        }
    }

    if (bSprintExhausted && Movement->MaxWalkSpeed > WalkSpeed)
    {
        Movement->MaxWalkSpeed = WalkSpeed;
    }

    if (FMath::Abs(Stamina - LastReportedStamina) >= 0.5f || Stamina <= 0.0f || Stamina >= MaxStamina)
    {
        LastReportedStamina = Stamina;
        BP_OnStaminaChanged(Stamina, MaxStamina, bSprintExhausted);
    }
}

void ARiftProductionPlayerCharacter::UpdateFirstPersonCameraMotion(float DeltaSeconds)
{
    if (!FirstPersonCamera)
    {
        return;
    }

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    const float Speed = GetVelocity().Size2D();
    const bool bGrounded = Movement && Movement->IsMovingOnGround();
    const bool bSprinting = Movement && Movement->MaxWalkSpeed > WalkSpeed + 5.0f && !bSprintExhausted && !CarriedSalvage;

    const float TargetHeight = bIsCrouched ? 42.0f : 64.0f;
    SmoothedCameraHeight = FMath::FInterpTo(SmoothedCameraHeight, TargetHeight, DeltaSeconds, CameraInterpSpeed);

    float BobZ = 0.0f;
    float BobY = 0.0f;
    if (bGrounded && Speed > 45.0f)
    {
        const float NormalizedSpeed = FMath::Clamp(Speed / FMath::Max(1.0f, SprintSpeed), 0.0f, 1.0f);
        BobPhase += DeltaSeconds * BobFrequency * FMath::Lerp(0.85f, 1.35f, NormalizedSpeed);
        const float Amplitude = bSprinting ? SprintBobAmplitude : WalkBobAmplitude;
        const float CrouchScale = bIsCrouched ? 0.45f : 1.0f;
        BobZ = FMath::Sin(BobPhase * 2.0f) * Amplitude * CrouchScale;
        BobY = FMath::Sin(BobPhase) * Amplitude * 0.32f * CrouchScale;
    }
    else
    {
        BobPhase = FMath::Fmod(BobPhase, PI * 2.0f);
    }

    FirstPersonCamera->SetRelativeLocation(FVector(0.0f, BobY, SmoothedCameraHeight + BobZ));

    const float DesiredFOV = FieldOfView + (bSprinting && Speed > WalkSpeed * 0.55f ? SprintFOVBoost : 0.0f);
    FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(
        FirstPersonCamera->FieldOfView,
        DesiredFOV,
        DeltaSeconds,
        7.5f));
}

FString ARiftProductionPlayerCharacter::GetSelectedBuildCostText() const
{
    switch (SelectedBuildPiece)
    {
        case ERiftBuildPiece::Platform: return TEXT("3 scrap");
        case ERiftBuildPiece::Beam: return TEXT("2 scrap");
        case ERiftBuildPiece::Wheel: return TEXT("1 scrap + 2 bolts");
        case ERiftBuildPiece::MotorWheel: return TEXT("2 scrap + 2 bolts + 1 motor + 1 electronics");
        default: return TEXT("materials");
    }
}

bool ARiftProductionPlayerCharacter::CanAffordSelectedBuildPiece() const
{
    switch (SelectedBuildPiece)
    {
        case ERiftBuildPiece::Platform:
            return Scrap >= 3;
        case ERiftBuildPiece::Beam:
            return Scrap >= 2;
        case ERiftBuildPiece::Wheel:
            return Scrap >= 1 && GetComponentCount(TEXT("bolts")) >= 2;
        case ERiftBuildPiece::MotorWheel:
            return Scrap >= 2 && GetComponentCount(TEXT("bolts")) >= 2 && GetComponentCount(TEXT("motor")) >= 1 && GetComponentCount(TEXT("electronics")) >= 1;
        default:
            return false;
    }
}

bool ARiftProductionPlayerCharacter::ConsumeSelectedBuildCost()
{
    if (!CanAffordSelectedBuildPiece())
    {
        return false;
    }

    switch (SelectedBuildPiece)
    {
        case ERiftBuildPiece::Platform:
            Scrap -= 3;
            break;
        case ERiftBuildPiece::Beam:
            Scrap -= 2;
            break;
        case ERiftBuildPiece::Wheel:
            Scrap -= 1;
            ConsumeComponentItem(TEXT("bolts"), 2);
            break;
        case ERiftBuildPiece::MotorWheel:
            Scrap -= 2;
            ConsumeComponentItem(TEXT("bolts"), 2);
            ConsumeComponentItem(TEXT("motor"), 1);
            ConsumeComponentItem(TEXT("electronics"), 1);
            break;
        default:
            return false;
    }

    BP_OnInventoryChanged();
    return true;
}

void ARiftProductionPlayerCharacter::ProductionBuildPlacePressed()
{
    if (!bBuildMode)
    {
        return;
    }

    if (!CanAffordSelectedBuildPiece())
    {
        CurrentInteractionText = FText::FromString(FString::Printf(
            TEXT("INSUFFICIENT MATERIALS | %s costs %s"),
            *StaticEnum<ERiftBuildPiece>()->GetNameStringByValue(static_cast<int64>(SelectedBuildPiece)),
            *GetSelectedBuildCostText()));
        return;
    }

    if (ConsumeSelectedBuildCost())
    {
        if (!PlaceBuildPiece())
        {
            // Refund only when spawning the final piece itself failed.
            switch (SelectedBuildPiece)
            {
                case ERiftBuildPiece::Platform: Scrap += 3; break;
                case ERiftBuildPiece::Beam: Scrap += 2; break;
                case ERiftBuildPiece::Wheel:
                    Scrap += 1;
                    AddComponentItem(TEXT("bolts"), 2);
                    break;
                case ERiftBuildPiece::MotorWheel:
                    Scrap += 2;
                    AddComponentItem(TEXT("bolts"), 2);
                    AddComponentItem(TEXT("motor"), 1);
                    AddComponentItem(TEXT("electronics"), 1);
                    break;
                default: break;
            }
            BP_OnInventoryChanged();
        }
    }
}

void ARiftProductionPlayerCharacter::UpdateBuildCostPrompt()
{
    if (!bBuildMode)
    {
        return;
    }

    const FString Existing = CurrentInteractionText.ToString();
    const FString Affordability = CanAffordSelectedBuildPiece() ? TEXT("READY") : TEXT("NEED MATERIALS");
    CurrentInteractionText = FText::FromString(FString::Printf(
        TEXT("%s | COST %s | %s"),
        *Existing,
        *GetSelectedBuildCostText(),
        *Affordability));
}

ARiftAssemblyPart* ARiftProductionPlayerCharacter::TraceEngineeringPart(FHitResult* OutHit) const
{
    if (!FirstPersonCamera || !GetWorld())
    {
        return nullptr;
    }

    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * EngineeringTraceDistance;
    FHitResult LocalHit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftEngineeringTrace), false, this);
    if (BuildPreview)
    {
        Params.AddIgnoredActor(BuildPreview);
    }
    if (CarriedSalvage)
    {
        Params.AddIgnoredActor(CarriedSalvage);
    }

    if (!GetWorld()->LineTraceSingleByChannel(LocalHit, Start, End, ECC_Visibility, Params))
    {
        return nullptr;
    }
    if (OutHit)
    {
        *OutHit = LocalHit;
    }
    return Cast<ARiftAssemblyPart>(LocalHit.GetActor());
}

FString ARiftProductionPlayerCharacter::EngineeringModeName() const
{
    const UEnum* Enum = StaticEnum<ERiftJointMode>();
    return Enum ? Enum->GetNameStringByValue(static_cast<int64>(SelectedJointMode)) : TEXT("Joint");
}

void ARiftProductionPlayerCharacter::EngineeringConnectPressed()
{
    if (bBuildMode)
    {
        CurrentInteractionText = FText::FromString(TEXT("Exit Build Mode before connecting parts"));
        return;
    }

    ARiftAssemblyPart* AimedPart = TraceEngineeringPart();
    if (!AimedPart)
    {
        CurrentInteractionText = FText::FromString(TEXT("Aim at a placed FAS part to connect it"));
        return;
    }

    if (!EngineeringSelectionA || !IsValid(EngineeringSelectionA))
    {
        EngineeringSelectionA = AimedPart;
        BP_OnEngineeringSelectionChanged(EngineeringSelectionA, SelectedJointMode);
        CurrentInteractionText = FText::FromString(FString::Printf(
            TEXT("%s selected | aim at second part and press G | T changes joint | X cancels"),
            *AimedPart->GetName()));
        return;
    }

    if (AimedPart == EngineeringSelectionA)
    {
        CancelEngineeringSelection();
        CurrentInteractionText = FText::FromString(TEXT("Engineering selection cancelled"));
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FVector JointLocation = (EngineeringSelectionA->GetActorLocation() + AimedPart->GetActorLocation()) * 0.5f;
    ARiftEngineeringJoint* Joint = GetWorld()->SpawnActor<ARiftEngineeringJoint>(
        ARiftEngineeringJoint::StaticClass(),
        JointLocation,
        FRotator::ZeroRotator,
        SpawnParams);

    if (!Joint)
    {
        CurrentInteractionText = FText::FromString(TEXT("Could not create engineering joint"));
        return;
    }

    Joint->Mode = SelectedJointMode;
    const bool bAttached = Joint->AttachActors(EngineeringSelectionA, AimedPart);
    if (!bAttached)
    {
        Joint->Destroy();
        CurrentInteractionText = FText::FromString(TEXT("Those parts cannot accept a physics joint"));
        return;
    }

    LastCreatedJoint = Joint;
    BP_OnEngineeringJointCreated(Joint);
    CurrentInteractionText = FText::FromString(FString::Printf(
        TEXT("%s joint created | G select another pair | T changes mode"),
        *EngineeringModeName()));
    EngineeringSelectionA = nullptr;
    BP_OnEngineeringSelectionChanged(nullptr, SelectedJointMode);
}

void ARiftProductionPlayerCharacter::CycleEngineeringJointMode(int32 Direction)
{
    constexpr int32 ModeCount = 4;
    int32 Value = static_cast<int32>(SelectedJointMode);
    Value = (Value + (Direction >= 0 ? 1 : -1) + ModeCount) % ModeCount;
    SelectedJointMode = static_cast<ERiftJointMode>(Value);
    BP_OnEngineeringSelectionChanged(EngineeringSelectionA, SelectedJointMode);
    CurrentInteractionText = FText::FromString(FString::Printf(
        TEXT("Engineering joint mode: %s"), *EngineeringModeName()));
}

void ARiftProductionPlayerCharacter::CancelEngineeringSelection()
{
    EngineeringSelectionA = nullptr;
    BP_OnEngineeringSelectionChanged(nullptr, SelectedJointMode);
}

void ARiftProductionPlayerCharacter::EngineeringModeNextPressed()
{
    CycleEngineeringJointMode(1);
}

void ARiftProductionPlayerCharacter::EngineeringCancelPressed()
{
    CancelEngineeringSelection();
    CurrentInteractionText = FText::FromString(TEXT("Engineering selection cleared"));
}

void ARiftProductionPlayerCharacter::UpdateEngineeringPrompt()
{
    if (bBuildMode)
    {
        return;
    }

    if (EngineeringSelectionA && !IsValid(EngineeringSelectionA))
    {
        EngineeringSelectionA = nullptr;
    }

    if (EngineeringSelectionA)
    {
        CurrentInteractionText = FText::FromString(FString::Printf(
            TEXT("ENGINEERING %s | first: %s | G second part | T mode | X cancel"),
            *EngineeringModeName(),
            *EngineeringSelectionA->GetName()));
    }
}
