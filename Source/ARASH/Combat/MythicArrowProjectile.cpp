#include "Combat/MythicArrowProjectile.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Player/ArashCharacter.h"
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

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowShaft"));
    VisualMesh->SetupAttachment(Collision);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetRelativeScale3D(FVector(1.05f, 0.045f, 0.045f));
    if (CubeAsset.Succeeded())
    {
        VisualMesh->SetStaticMesh(CubeAsset.Object);
    }

    ArrowHeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowHead"));
    ArrowHeadMesh->SetupAttachment(Collision);
    ArrowHeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ArrowHeadMesh->SetRelativeLocation(FVector(62.0f, 0.0f, 0.0f));
    ArrowHeadMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    ArrowHeadMesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.24f));
    if (ConeAsset.Succeeded())
    {
        ArrowHeadMesh->SetStaticMesh(ConeAsset.Object);
    }

    if (BasicMaterial.Succeeded())
    {
        VisualMesh->SetMaterial(0, BasicMaterial.Object);
        ArrowHeadMesh->SetMaterial(0, BasicMaterial.Object);
    }

    GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ArrowGlow"));
    GlowLight->SetupAttachment(Collision);
    GlowLight->SetIntensity(2200.0f);
    GlowLight->SetAttenuationRadius(260.0f);
    GlowLight->SetLightColor(FLinearColor(1.0f, 0.48f, 0.06f));
    GlowLight->SetCastShadows(false);

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

    VisualMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.95f, 0.58f, 0.08f));
    ArrowHeadMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.98f, 0.82f, 0.30f));

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
        const FVector TrailEnd = GetActorLocation() - Movement->Velocity.GetSafeNormal() * 180.0f;
        const FColor TrailColor = bReturning ? FColor(40, 220, 235) : FColor(255, 170, 35);
        DrawDebugLine(GetWorld(), GetActorLocation(), TrailEnd, TrailColor, false, 0.06f, 0, 5.0f);
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

    VisualMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.02f, 0.72f, 0.82f));
    ArrowHeadMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.34f, 0.95f, 1.0f));
    if (GlowLight)
    {
        GlowLight->SetLightColor(FLinearColor(0.02f, 0.75f, 0.95f));
        GlowLight->SetIntensity(2800.0f);
    }
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

    if (AArashCharacter* Player = Cast<AArashCharacter>(GetOwner()))
    {
        Player->PlayCombatFeedback(bReturning ? 0.28f : 0.48f, !bReturning);
    }

#if WITH_EDITOR
    DrawDebugSphere(GetWorld(), GetActorLocation(), 42.0f, 10, FColor(255, 190, 60), false, 0.10f, 0, 3.0f);
#endif

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

    if (AArashCharacter* Player = Cast<AArashCharacter>(GetOwner()))
    {
        Player->PlayCombatFeedback(0.16f, false);
    }

#if WITH_EDITOR
    DrawDebugSphere(GetWorld(), ImpactResult.ImpactPoint, 28.0f, 8, FColor(70, 210, 220), false, 0.08f, 0, 2.0f);
#endif

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
