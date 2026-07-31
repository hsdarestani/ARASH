#include "Game/ArashGameModeBase.h"

#include "Player/ArashCharacter.h"

AArashGameModeBase::AArashGameModeBase()
{
    DefaultPawnClass = AArashCharacter::StaticClass();
}
