#include "Enemies/EnemyBoltProjectile.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Player/ArashCharacter.h"
#include "UObject/ConstructorHelpers.h"

AEnemyBoltProjectile::AEnemyBoltProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(18.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    RootComponent = Collision;

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    VisualMesh->SetupAttachment(Collision);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetRelativeScale3D(FVector(0.24f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereAsset.Succeeded())
    {
        VisualMesh->SetStaticMesh(SphereAsset.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BasicMaterial.Succeeded())
    {
        VisualMesh->SetMaterial(0, BasicMaterial.Object);
    }

    GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
    GlowLight->SetupAttachment(Collision);
    GlowLight->SetIntensity(1800.0f);
    GlowLight->SetAttenuationRadius(240.0f);
    GlowLight->SetLightColor(FLinearColor(1.0f, 0.12f, 0.035f));
    GlowLight->SetCastShadows(false);

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->UpdatedComponent = Collision;
    Movement->InitialSpeed = 1050.0f;
    Movement->MaxSpeed = 1050.0f;
    Movement->ProjectileGravityScale = 0.0f;
    Movement->bRotationFollowsVelocity = true;

    InitialLifeSpan = 4.0f;
}

void AEnemyBoltProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* BoltOwner = GetOwner())
    {
        Collision->IgnoreActorWhenMoving(BoltOwner, true);
    }

    VisualMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.95f, 0.20f, 0.08f));

    Collision->OnComponentBeginOverlap.AddDynamic(this, &AEnemyBoltProjectile::OnBoltOverlap);
    Collision->OnComponentHit.AddDynamic(this, &AEnemyBoltProjectile::OnBoltHit);
}

void AEnemyBoltProjectile::OnBoltOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult)
{
    AArashCharacter* Player = Cast<AArashCharacter>(OtherActor);
    if (!Player)
    {
        return;
    }

    UGameplayStatics::ApplyDamage(Player, Damage, GetInstigatorController(), this, UDamageType::StaticClass());
    Destroy();
}

void AEnemyBoltProjectile::OnBoltHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherActor != GetOwner())
    {
        Destroy();
    }
}
