#include "Game/ArashGameModeBase.h"

#include "Enemies/ArashEnemyBase.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ArashCharacter.h"

AArashGameModeBase::AArashGameModeBase()
{
    DefaultPawnClass = AArashCharacter::StaticClass();
    EnemyClass = AArashEnemyBase::StaticClass();
}

void AArashGameModeBase::BeginPlay()
{
    Super::BeginPlay();
    SpawnPrototypeEnemies();
}

void AArashGameModeBase::SpawnPrototypeEnemies()
{
    UWorld* World = GetWorld();
    if (!World || !EnemyClass)
    {
        return;
    }

    const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    const FVector Center = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector(0.0f, 0.0f, 100.0f);

    const FVector Offsets[] =
    {
        FVector(650.0f, 0.0f, 0.0f),
        FVector(-650.0f, 0.0f, 0.0f),
        FVector(0.0f, 650.0f, 0.0f),
        FVector(0.0f, -650.0f, 0.0f),
        FVector(520.0f, 520.0f, 0.0f),
        FVector(-520.0f, -520.0f, 0.0f)
    };

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    for (const FVector& Offset : Offsets)
    {
        World->SpawnActor<AArashEnemyBase>(EnemyClass, Center + Offset, FRotator::ZeroRotator, Params);
    }
}
