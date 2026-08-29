#include "RiftProductionPlayer.h"

#include "Camera/CameraComponent.h"
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
}

void ARiftProductionPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateStamina(DeltaSeconds);
    UpdateFirstPersonCameraMotion(DeltaSeconds);
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
