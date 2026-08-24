// Copyright Isaac Turpin


#include "Player/ShooterPlayerState.h"

#include "Data/SpecialElimData.h"
#include "UI/Elims/SpecialElim.h"

AShooterPlayerState::AShooterPlayerState()
{
	SetNetUpdateFrequency(100.f);
	
	ScoredElims = 0;
	Defeats = 0;
	Hits = 0;
	Misses = 0;
	bOnStreak = false;
	HeadShotElims = 0;
	HighestStreak = 0;
	RevengeElims = 0;
	DethroneElims = 0;
	ShowStopperElims = 0;
	bFirstBlood = false;
	bWinner = false;
	bIsProcessingQueue = false;
	ElimDisplayTime = 0.5f;
}

void AShooterPlayerState::AddScoredElim()
{
	++ScoredElims;
}

void AShooterPlayerState::AddDefeat()
{
	++Defeats;
}

void AShooterPlayerState::AddHit()
{
	++Hits;
}

void AShooterPlayerState::AddMiss()
{
	++Misses;
}

void AShooterPlayerState::AddHeadshotElim()
{
	++HeadShotElims;
}

void AShooterPlayerState::AddSequentialElim(int32 SequenceCount)
{
	if (SequentialElims.Contains(SequenceCount))
	{
		SequentialElims[SequenceCount]++;
	}
	else
	{
		SequentialElims.Add(SequenceCount, 1);
	}
	
	for (auto& Elim : SequentialElims)
	{
		if (Elim.Key < SequenceCount && Elim.Value > 0)
		{
			Elim.Value--;
		}
	}
}

void AShooterPlayerState::UpdateHighestStreak(int32 StreakCount)
{
	if (StreakCount > HighestStreak)
	{
		HighestStreak = StreakCount;
	}
}

void AShooterPlayerState::AddRevengeElim()
{
	++RevengeElims;
}

void AShooterPlayerState::AddDethroneElim()
{
	++DethroneElims;
}

void AShooterPlayerState::AddShowStopperElim()
{
	++ShowStopperElims;
}

void AShooterPlayerState::GotFirstBlood()
{
	bFirstBlood = true;
}

void AShooterPlayerState::IsNowWinner()
{
	bWinner = true;
}

void AShooterPlayerState::SetOnStreak(bool bIsOnStreak)
{
	bOnStreak = bIsOnStreak;
}

void AShooterPlayerState::SetLastAttacker(APlayerState* Attacker)
{
	LastAttacker = Attacker;
}

bool AShooterPlayerState::IsOnStreak() const
{
	return bOnStreak;
}

APlayerState* AShooterPlayerState::GetLastAttacker() const
{
	return LastAttacker.IsValid() ? LastAttacker.Get() : nullptr;
}

int32 AShooterPlayerState::GetScoredElims() const
{
	return ScoredElims;
}

TArray<ESpecialElimType> AShooterPlayerState::DecodeElimBitmask(ESpecialElimType ElimTypeBitmask)
{
	TArray<ESpecialElimType> ValidElims;
	
	uint16 BitmaskValue = static_cast<uint16>(ElimTypeBitmask);
	
	for (uint16 i = 0; i < 16; i++)
	{
		if (BitmaskValue & (1 << i))
		{
			ESpecialElimType EnumValue = static_cast<ESpecialElimType>( 1 << i);
			ValidElims.Add(EnumValue);
		}
	}
	
	return ValidElims;
}

void AShooterPlayerState::Client_ScoredElim_Implementation(int32 ElimScore)
{
	OnScoreChanged.Broadcast(ElimScore);
}

void AShooterPlayerState::Client_SpecialElim_Implementation(const ESpecialElimType& SpecialElim,
	int32 SequentialElimCount, int32 StreakCount, int32 ElimScore)
{
	ensure(IsValid(SpecialElimData));
	
	OnScoreChanged.Broadcast(ElimScore);
	
	TArray<ESpecialElimType> ElimTypes = DecodeElimBitmask(SpecialElim);
	for (ESpecialElimType ElimType : ElimTypes)
	{
		FSpecialElimInfo& ElimMessageInfo = SpecialElimData->SpecialElimInfo.FindChecked(ElimType);
		if (ElimType == ESpecialElimType::Sequential)
		{
			ElimMessageInfo.SequentialElimCount = SequentialElimCount;
		}
		if (ElimType == ESpecialElimType::Streak)
		{
			ElimMessageInfo.StreakCount = StreakCount;
		}
		ElimMessageInfo.ElimType = ElimType;
		
		SpecialElimQueue.Enqueue(ElimMessageInfo);
	}
	if (!bIsProcessingQueue)
	{
		ProcessNextSpecialElim();
	}
}

void AShooterPlayerState::ProcessNextSpecialElim()
{
	FSpecialElimInfo ElimInfo;
	if (SpecialElimQueue.Dequeue(ElimInfo))
	{
		bIsProcessingQueue = true;
		ShowSpecialElim(ElimInfo);
		
		GetWorldTimerManager().SetTimerForNextTick([this]()
		{
			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(TimerHandle, this, &AShooterPlayerState::ProcessNextSpecialElim, ElimDisplayTime, false);
		});
	}
	else
	{
		bIsProcessingQueue = false;
	}
}

void AShooterPlayerState::ShowSpecialElim(const FSpecialElimInfo& ElimMessageInfo)
{
	FString ElimMessageString = ElimMessageInfo.ElimMessage;
	if (ElimMessageInfo.ElimType == ESpecialElimType::Sequential)
	{
		if (ElimMessageInfo.SequentialElimCount == 2) ElimMessageString = FString("Double Elim!");
		else if (ElimMessageInfo.SequentialElimCount == 3) ElimMessageString = FString("Triple Elim!");
		else if (ElimMessageInfo.SequentialElimCount == 4) ElimMessageString = FString("Quad Elim!");
		else if (ElimMessageInfo.SequentialElimCount > 4) ElimMessageString = FString::Printf(TEXT("Rampage x%d!"), ElimMessageInfo.SequentialElimCount);
	}
	if (ElimMessageInfo.ElimType == ESpecialElimType::Streak) ElimMessageString = FString::Printf(TEXT("Streak x%d!"), ElimMessageInfo.StreakCount);
	
	if (IsValid(SpecialElimWidgetClass))
	{
		USpecialElim* ElimWidget = CreateWidget<USpecialElim>(GetPlayerController(), SpecialElimWidgetClass);
		if (IsValid(ElimWidget))
		{
			ElimWidget->InitialiseWidget(ElimMessageString, ElimMessageInfo.ElimIcon);
			ElimWidget->AddToViewport();
		}
	}
}

void AShooterPlayerState::Client_LostTheLead_Implementation()
{
	ensure(IsValid(SpecialElimData));
	FSpecialElimInfo& ElimMessageInfo = SpecialElimData->SpecialElimInfo.FindChecked(ESpecialElimType::LostTheLead);
	
	if (IsValid(SpecialElimWidgetClass))
	{
		USpecialElim* ElimWidget = CreateWidget<USpecialElim>(GetPlayerController(), SpecialElimWidgetClass);
		if (IsValid(ElimWidget))
		{
			ElimWidget->InitialiseWidget(ElimMessageInfo.ElimMessage, ElimMessageInfo.ElimIcon);
			ElimWidget->AddToViewport();
		}
	}
}
