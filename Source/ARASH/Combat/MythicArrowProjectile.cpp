#include "Combat/MythicArrowProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

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

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->UpdatedComponent = Collision;
    Movement->InitialSpeed = 2400.0f;
    Movement->MaxSpeed = 3200.0f;
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

    const FVector DesiredVelocity = ToOwner.GetSafeNormal() * Movement->MaxSpeed;
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
