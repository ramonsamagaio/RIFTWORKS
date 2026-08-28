#pragma once

#include "CoreMinimal.h"

// Stable runtime compatibility umbrella for the small native foundation.
// Gameplay/content remains Blueprint-facing; this header keeps the native layer
// independent of unity-build/transitive-include accidents across UE 5.8 builds.
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/GameInstance.h"
#include "Engine/Scene.h"
#include "Navigation/PathFollowingComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
