#include "Game/ArashHUD.h"

#include "Combat/ArashBowComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Game/ArashGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ArashCharacter.h"

void AArashHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas || !GEngine)
    {
        return;
    }

    const AArashCharacter* Player = Cast<AArashCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    const AArashGameModeBase* GameMode = Cast<AArashGameModeBase>(UGameplayStatics::GetGameMode(this));
    if (!Player || !GameMode)
    {
        return;
    }

    const float ScreenW = Canvas->SizeX;
    const float ScreenH = Canvas->SizeY;
    UFont* Font = GEngine->GetSmallFont();

    DrawText(TEXT("ARASH"), FLinearColor::White, 36.0f, 30.0f, Font, 1.6f, false);
    DrawText(FString::Printf(TEXT("WAVE %d"), GameMode->GetCurrentWave()), FLinearColor::White, ScreenW - 170.0f, 34.0f, Font, 1.25f, false);
    DrawText(FString::Printf(TEXT("ENEMIES %d"), GameMode->GetEnemiesRemaining()), FLinearColor(0.85f, 0.85f, 0.85f, 1.0f), ScreenW - 170.0f, 58.0f, Font, 1.0f, false);

    DrawText(FString::Printf(TEXT("HP %.0f / %.0f"), Player->CurrentHealth, Player->MaxHealth), FLinearColor::White, 36.0f, ScreenH - 88.0f, Font, 1.0f, false);
    DrawBar(36.0f, ScreenH - 60.0f, 280.0f, 18.0f, Player->GetHealthAlpha(), FLinearColor(0.72f, 0.08f, 0.06f, 1.0f));

    if (Player->Bow)
    {
        const float Charge = Player->Bow->GetChargeAlpha();
        if (Charge > KINDA_SMALL_NUMBER)
        {
            DrawText(TEXT("BOW CHARGE"), FLinearColor::White, ScreenW * 0.5f - 78.0f, ScreenH - 72.0f, Font, 0.9f, false);
            DrawBar(ScreenW * 0.5f - 120.0f, ScreenH - 48.0f, 240.0f, 12.0f, Charge, FLinearColor(0.95f, 0.58f, 0.08f, 1.0f));
        }
    }

    if (GameMode->IsWaitingForUpgrade())
    {
        DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f), ScreenW * 0.5f - 330.0f, ScreenH * 0.5f - 150.0f, 660.0f, 300.0f);
        DrawText(TEXT("WAVE CLEARED - CHOOSE A BLESSING"), FLinearColor(1.0f, 0.74f, 0.2f, 1.0f), ScreenW * 0.5f - 230.0f, ScreenH * 0.5f - 112.0f, Font, 1.25f, false);
        DrawText(TEXT("[1] ZAHHAK'S FANG   +1 PIERCE"), FLinearColor::White, ScreenW * 0.5f - 220.0f, ScreenH * 0.5f - 45.0f, Font, 1.05f, false);
        DrawText(TEXT("[2] SIMURGH'S WING  +1 RICOCHET"), FLinearColor::White, ScreenW * 0.5f - 220.0f, ScreenH * 0.5f + 5.0f, Font, 1.05f, false);
        DrawText(TEXT("[3] ARASH'S OATH     +25% RETURN SPEED"), FLinearColor::White, ScreenW * 0.5f - 220.0f, ScreenH * 0.5f + 55.0f, Font, 1.05f, false);
    }

    if (GameMode->IsGameOver())
    {
        DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f), ScreenW * 0.5f - 250.0f, ScreenH * 0.5f - 90.0f, 500.0f, 180.0f);
        DrawText(TEXT("ARASH HAS FALLEN"), FLinearColor(0.9f, 0.18f, 0.12f, 1.0f), ScreenW * 0.5f - 145.0f, ScreenH * 0.5f - 45.0f, Font, 1.4f, false);
        DrawText(TEXT("PRESS R TO RESTART"), FLinearColor::White, ScreenW * 0.5f - 120.0f, ScreenH * 0.5f + 18.0f, Font, 1.0f, false);
    }
}

void AArashHUD::DrawBar(float X, float Y, float Width, float Height, float Alpha, const FLinearColor& FillColor)
{
    if (!Canvas)
    {
        return;
    }

    const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
    DrawRect(FLinearColor(0.03f, 0.03f, 0.03f, 0.9f), X, Y, Width, Height);
    DrawRect(FillColor, X + 2.0f, Y + 2.0f, (Width - 4.0f) * ClampedAlpha, Height - 4.0f);
}
