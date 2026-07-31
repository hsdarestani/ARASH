#include "Game/ArashGameModeBase.h"

#include "Enemies/ArashEnemyBase.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ArashCharacter.h"
#include "UObject/ConstructorHelpers.h"

AArashGameModeBase::AArashGameModeBase()
{
    DefaultPawnClass = AArashCharacter::StaticClass();
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
    SpawnPrototypeArena();
    SpawnPrototypeEnemies();
}

void AArashGameModeBase::SpawnPrototypeArena()
{
    UWorld* World = GetWorld();
    if (!World || !PrototypeCubeMesh)
    {
        return;
    }

    const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    const FVector Center = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;
    constexpr float HalfExtent = 1450.0f;
    constexpr float WallHeight = 300.0f;

    struct FWallSpec
    {
        FVector Location;
        FVector Scale;
    };

    const FWallSpec Walls[] =
    {
        {Center + FVector(HalfExtent, 0.0f, WallHeight * 0.5f), FVector(1.0f, 29.0f, 3.0f)},
        {Center + FVector(-HalfExtent, 0.0f, WallHeight * 0.5f), FVector(1.0f, 29.0f, 3.0f)},
        {Center + FVector(0.0f, HalfExtent, WallHeight * 0.5f), FVector(29.0f, 1.0f, 3.0f)},
        {Center + FVector(0.0f, -HalfExtent, WallHeight * 0.5f), FVector(29.0f, 1.0f, 3.0f)}
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
        FVector(850.0f, 0.0f, 0.0f),
        FVector(-850.0f, 0.0f, 0.0f),
        FVector(0.0f, 850.0f, 0.0f),
        FVector(0.0f, -850.0f, 0.0f),
        FVector(650.0f, 650.0f, 0.0f),
        FVector(-650.0f, -650.0f, 0.0f)
    };

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    for (const FVector& Offset : Offsets)
    {
        World->SpawnActor<AArashEnemyBase>(EnemyClass, Center + Offset, FRotator::ZeroRotator, Params);
    }
}
