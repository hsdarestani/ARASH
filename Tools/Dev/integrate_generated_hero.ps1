$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Target = Join-Path $RepoRoot "Source\ARASH\Player\ArashCharacter.cpp"

if (-not (Test-Path $Target)) {
    throw "Character source file was not found: $Target"
}

$Content = Get-Content -Path $Target -Raw
$Original = $Content

$FinderAnchor = '    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));'
$FinderBlock = @'
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeroBodyAsset(TEXT("/Game/Art/Generated/Character/SM_ARASH_HeroBody_A.SM_ARASH_HeroBody_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeroHeadAsset(TEXT("/Game/Art/Generated/Character/SM_ARASH_HeroHead_A.SM_ARASH_HeroHead_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeroCapeAsset(TEXT("/Game/Art/Generated/Character/SM_ARASH_HeroCape_A.SM_ARASH_HeroCape_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeroLeftArmAsset(TEXT("/Game/Art/Generated/Character/SM_ARASH_HeroLeftArm_A.SM_ARASH_HeroLeftArm_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeroRightArmAsset(TEXT("/Game/Art/Generated/Character/SM_ARASH_HeroRightArm_A.SM_ARASH_HeroRightArm_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeroBowUpperAsset(TEXT("/Game/Art/Generated/Character/SM_ARASH_HeroBowUpper_A.SM_ARASH_HeroBowUpper_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeroBowLowerAsset(TEXT("/Game/Art/Generated/Character/SM_ARASH_HeroBowLower_A.SM_ARASH_HeroBowLower_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeroStringUpperAsset(TEXT("/Game/Art/Generated/Character/SM_ARASH_HeroStringUpper_A.SM_ARASH_HeroStringUpper_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeroStringLowerAsset(TEXT("/Game/Art/Generated/Character/SM_ARASH_HeroStringLower_A.SM_ARASH_HeroStringLower_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeroArrowAsset(TEXT("/Game/Art/Generated/Character/SM_ARASH_HeroArrow_A.SM_ARASH_HeroArrow_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeroQuiverAsset(TEXT("/Game/Art/Generated/Character/SM_ARASH_HeroQuiver_A.SM_ARASH_HeroQuiver_A"));
'@

if (-not $Content.Contains('HeroBodyAsset(TEXT(')) {
    if (-not $Content.Contains($FinderAnchor)) {
        throw "Could not find the character asset anchor."
    }
    $Content = $Content.Replace($FinderAnchor, $FinderBlock.TrimEnd("`r", "`n"))
}

$CameraAnchor = '    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));'
$SetupBlock = @'
    const bool bGeneratedHeroAvailable =
        HeroBodyAsset.Succeeded() && HeroHeadAsset.Succeeded() && HeroCapeAsset.Succeeded() &&
        HeroLeftArmAsset.Succeeded() && HeroRightArmAsset.Succeeded() &&
        HeroBowUpperAsset.Succeeded() && HeroBowLowerAsset.Succeeded() &&
        HeroStringUpperAsset.Succeeded() && HeroStringLowerAsset.Succeeded() &&
        HeroArrowAsset.Succeeded() && HeroQuiverAsset.Succeeded();

    if (bGeneratedHeroAvailable)
    {
        VisualMesh->SetStaticMesh(HeroBodyAsset.Object);
        HeadMesh->SetStaticMesh(HeroHeadAsset.Object);
        MantleMesh->SetStaticMesh(HeroCapeAsset.Object);
        LeftArmMesh->SetStaticMesh(HeroLeftArmAsset.Object);
        RightArmMesh->SetStaticMesh(HeroRightArmAsset.Object);
        BowUpperMesh->SetStaticMesh(HeroBowUpperAsset.Object);
        BowLowerMesh->SetStaticMesh(HeroBowLowerAsset.Object);
        BowStringUpperMesh->SetStaticMesh(HeroStringUpperAsset.Object);
        BowStringLowerMesh->SetStaticMesh(HeroStringLowerAsset.Object);
        NockedArrowMesh->SetStaticMesh(HeroArrowAsset.Object);
        QuiverMesh->SetStaticMesh(HeroQuiverAsset.Object);

        VisualMesh->SetRelativeLocation(FVector::ZeroVector);
        VisualMesh->SetRelativeScale3D(FVector::OneVector);
        HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
        HeadMesh->SetRelativeScale3D(FVector::OneVector);
        MantleMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 58.0f));
        MantleMesh->SetRelativeScale3D(FVector::OneVector);
        LeftArmMesh->SetRelativeScale3D(FVector::OneVector);
        RightArmMesh->SetRelativeScale3D(FVector::OneVector);
        BowUpperMesh->SetRelativeScale3D(FVector::OneVector);
        BowLowerMesh->SetRelativeScale3D(FVector::OneVector);
        BowStringUpperMesh->SetRelativeScale3D(FVector::OneVector);
        BowStringLowerMesh->SetRelativeScale3D(FVector::OneVector);
        NockedArrowMesh->SetRelativeScale3D(FVector::OneVector);
        QuiverMesh->SetRelativeLocation(FVector(-23.0f, 32.0f, 28.0f));
        QuiverMesh->SetRelativeRotation(FRotator(18.0f, 0.0f, 0.0f));
        QuiverMesh->SetRelativeScale3D(FVector::OneVector);

        UE_LOG(LogTemp, Display, TEXT("[ARASH Hero] Generated modular hero kit loaded."));
    }

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
'@

if (-not $Content.Contains('[ARASH Hero] Generated modular hero kit loaded.')) {
    if (-not $Content.Contains($CameraAnchor)) {
        throw "Could not find the character camera anchor."
    }
    $Content = $Content.Replace($CameraAnchor, $SetupBlock.TrimEnd("`r", "`n"))
}

if ($Content -eq $Original) {
    Write-Host "[ARASH] Generated hero integration is already applied."
    exit 0
}

$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($Target, $Content, $Utf8NoBom)

$Verify = Get-Content -Path $Target -Raw
if (-not $Verify.Contains('HeroBodyAsset(TEXT(') -or
    -not $Verify.Contains('[ARASH Hero] Generated modular hero kit loaded.')) {
    throw "Generated hero integration verification failed."
}

Write-Host "[ARASH] Generated hero integration applied:"
Write-Host $Target
