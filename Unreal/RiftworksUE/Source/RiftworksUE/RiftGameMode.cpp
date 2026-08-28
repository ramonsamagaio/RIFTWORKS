#include "RiftGameMode.h"
#include "RiftHUD.h"
#include "RiftPlayerCharacter.h"

ARiftGameMode::ARiftGameMode()
{
    DefaultPawnClass = ARiftPlayerCharacter::StaticClass();
    HUDClass = ARiftHUD::StaticClass();
}
