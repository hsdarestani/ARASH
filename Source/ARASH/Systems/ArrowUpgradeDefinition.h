#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArrowUpgradeDefinition.generated.h"

UENUM(BlueprintType)
enum class EArrowUpgradeType : uint8
{
    Damage,
    Speed,
    Pierce,
    Ricochet,
    Return,
    Split,
    Burn,
    Chain,
    Explosion
};

UCLASS(BlueprintType)
class ARASH_API UArrowUpgradeDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
    FName UpgradeId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
    EArrowUpgradeType Type = EArrowUpgradeType::Damage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
    float Magnitude = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "1"))
    int32 MaxStacks = 5;
};
