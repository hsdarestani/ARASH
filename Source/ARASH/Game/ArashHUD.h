#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ArashHUD.generated.h"

UCLASS()
class ARASH_API AArashHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    void DrawBar(float X, float Y, float Width, float Height, float Alpha, const FLinearColor& FillColor);
};
