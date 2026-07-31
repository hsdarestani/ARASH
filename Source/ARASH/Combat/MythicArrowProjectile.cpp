#include "Combat/MythicArrowProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AMythicArrowProjectile::AMythicArrowProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(10.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    RootComponent = Collision;

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeVisual"));
    VisualMesh->SetupAttachment(Collision);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetRelativeScale3D(FVector(1.35f, 0.06f, 0.06f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ArrowVisualAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (ArrowVisualAsset.Succeeded())
    {
        VisualMesh->SetStaticMesh(ArrowVisualAsset.Object);
    }

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->UpdatedComponent = Collision;
    Movement->InitialSpeed = 2100.0f;
    Movement->MaxSpeed = 3000.0f;
    Movement->ProjectileGravityScale = 0.0f;
    Movement->bRotationFollowsVelocity = true;
    Movement->bShouldBounce = true;
    Movement->Bounciness = 1.0f;
    Movement->Friction = 0.0f;

    InitialLifeSpan = 8.0f;
}

void AMythicArrowProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* ArrowOwner = GetOwner())
    {
        Collision->IgnoreActorWhenMoving(ArrowOwner, true);
    }

    Collision->OnComponentBeginOverlap.AddDynamic(this, &AMythicArrowProjectile::OnArrowOverlap);
    Movement->OnProjectileBounce.AddDynamic(this, &AMythicArrowProjectile::OnProjectileBounce);

    if (bReturnToOwner)
    {
        FTimerHandle ReturnTimer;
        GetWorldTimerManager().SetTimer(ReturnTimer, this, &AMythicArrowProjectile::BeginReturn, ReturnDelay, false);
    }
}

void AMythicArrowProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

#if WITH_EDITOR
    if (GetWorld() && Movement && !Movement->Velocity.IsNearlyZero())
    {
        const FVector TrailEnd = GetActorLocation() - Movement->Velocity.GetSafeNormal() * 150.0f;
        DrawDebugLine(GetWorld(), GetActorLocation(), TrailEnd, FColor(255, 170, 35), false, 0.06f, 0, 4.0f);
    }
#endif

    if (!bReturning || !IsValid(GetOwner()))
    {
        return;
    }

    const FVector ToOwner = GetOwner()->GetActorLocation() - GetActorLocation();
    if (ToOwner.SizeSquared() < FMath::Square(90.0f))
    {
        Destroy();
        return;
    }

    const FVector DesiredVelocity = ToOwner.GetSafeNormal() * Movement->MaxSpeed * ReturnSpeedMultiplier;
    Movement->Velocity = FMath::VInterpTo(Movement->Velocity, DesiredVelocity, DeltaSeconds, 8.0f);
}

void AMythicArrowProjectile::BeginReturn()
{
    if (!bReturnToOwner || !IsValid(GetOwner()))
    {
        return;
    }

    bReturning = true;
    HitActors.Reset();
    Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
}

void AMythicArrowProjectile::OnArrowOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!IsValid(OtherActor) || OtherActor == this || OtherActor == GetOwner())
    {
        return;
    }

    if (HitActors.Contains(OtherActor))
    {
        return;
    }

    HitActors.Add(OtherActor);

    UGameplayStatics::ApplyDamage(
        OtherActor,
        BaseDamage,
        GetInstigatorController(),
        this,
        UDamageType::StaticClass());

    if (!bReturning)
    {
        ++PierceCount;
        if (PierceCount > MaxPierces)
        {
            if (bReturnToOwner)
            {
                BeginReturn();
            }
            else
            {
                Destroy();
            }
        }
    }
}

void AMythicArrowProjectile::OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
    if (bReturning)
    {
        return;
    }

    ++BounceCount;
    if (BounceCount > MaxBounces)
    {
        if (bReturnToOwner)
        {
            BeginReturn();
        }
        else
        {
            Destroy();
        }
    }
}
