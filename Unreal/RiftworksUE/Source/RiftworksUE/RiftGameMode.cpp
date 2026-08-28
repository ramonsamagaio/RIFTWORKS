#include "RiftGameMode.h"
#include "RiftPlayerCharacter.h"

ARiftGameMode::ARiftGameMode()
{
    DefaultPawnClass = ARiftPlayerCharacter::StaticClass();
}
