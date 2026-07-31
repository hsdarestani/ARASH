#include "Game/ArashGameModeBase.h"

#include "Enemies/ArashEnemyBase.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Game/ArashHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ArashCharacter.h"
#include "UObject/ConstructorHelpers.h"

AArashGameModeBase::AArashGameModeBase()
{
    DefaultPawnClass = AArashCharacter::StaticClass();
    HUDClass = AArashHUD::StaticClass();
    EnemyClass = AArashEnemyBase::StaticClass();

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeAsset.Succeeded())
    {
        PrototypeCubeMesh = CubeAsset.Object;
    }
}

void AArashGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        ArenaCenter = PlayerPawn->GetActorLocation();
    }

    SpawnPrototypeArena();
    StartWave();
}

void AArashGameModeBase::SpawnPrototypeArena()
{
    UWorld* World = GetWorld();
    if (!World || !PrototypeCubeMesh)
    {
        return;
    }

    constexpr float HalfExtent = 1450.0f;
    constexpr float WallHeight = 300.0f;

    struct FWallSpec
    {
        FVector Location;
        FVector Scale;
    };

    const FWallSpec Walls[] =
    {
        {ArenaCenter + FVector(HalfExtent, 0.0f, WallHeight * 0.5f), FVector(1.0f, 29.0f, 3.0f)},
        {ArenaCenter + FVector(-HalfExtent, 0.0f, WallHeight * 0.5f), FVector(1.0f, 29.0f, 3.0f)},
        {ArenaCenter + FVector(0.0f, HalfExtent, WallHeight * 0.5f), FVector(29.0f, 1.0f, 3.0f)},
        {ArenaCenter + FVector(0.0f, -HalfExtent, WallHeight * 0.5f), FVector(29.0f, 1.0f, 3.0f)}
    };

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    for (const FWallSpec& Spec : Walls)
    {
        AStaticMeshActor* Wall = World->SpawnActor<AStaticMeshActor>(Spec.Location, FRotator::ZeroRotator, Params);
        if (!Wall)
        {
            continue;
        }

        Wall->GetStaticMeshComponent()->SetStaticMesh(PrototypeCubeMesh);
        Wall->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
        Wall->SetActorScale3D(Spec.Scale);
    }
}

void AArashGameModeBase::StartWave()
{
    if (bGameOver)
    {
        return;
    }

    bWaitingForUpgrade = false;
    SpawnWaveEnemies(CurrentWave);
}

void AArashGameModeBase::SpawnWaveEnemies(int32 WaveNumber)
{
    UWorld* World = GetWorld();
    if (!World || !EnemyClass)
    {
        return;
    }

    const int32 EnemyCount = FMath::Clamp(4 + WaveNumber * 2, 6, 18);
    EnemiesRemaining = 0;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    for (int32 Index = 0; Index < EnemyCount; ++Index)
    {
        const float Angle = (TWO_PI * static_cast<float>(Index)) / static_cast<float>(EnemyCount);
        const float Radius = (Index % 2 == 0) ? 820.0f : 1040.0f;
        const FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);

        AArashEnemyBase* Enemy = World->SpawnActor<AArashEnemyBase>(EnemyClass, ArenaCenter + Offset, FRotator::ZeroRotator, Params);
        if (Enemy)
        {
            Enemy->ConfigureForWave(WaveNumber);
            ++EnemiesRemaining;
        }
    }
}

void AArashGameModeBase::NotifyEnemyKilled(AArashEnemyBase* Enemy)
{
    if (bGameOver || bWaitingForUpgrade)
    {
        return;
    }

    EnemiesRemaining = FMath::Max(0, EnemiesRemaining - 1);
    if (EnemiesRemaining == 0)
    {
        bWaitingForUpgrade = true;
    }
}

void AArashGameModeBase::NotifyPlayerDied()
{
    bGameOver = true;
    bWaitingForUpgrade = false;
}

void AArashGameModeBase::SelectUpgrade(int32 UpgradeIndex)
{
    if (!bWaitingForUpgrade || bGameOver)
    {
        return;
    }

    AArashCharacter* Player = Cast<AArashCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Player || Player->IsDead())
    {
        return;
    }

    switch (UpgradeIndex)
    {
        case 0:
            Player->ApplyPierceUpgrade();
            break;
        case 1:
            Player->ApplyRicochetUpgrade();
            break;
        case 2:
            Player->ApplyReturnUpgrade();
            break;
        default:
            return;
    }

    ++CurrentWave;
    StartWave();
}
