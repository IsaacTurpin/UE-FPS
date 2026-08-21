// Copyright Isaac Turpin


#include "Elimination/EliminationComponent.h"
#include "GameFramework/Pawn.h"
#include "Player/ShooterPlayerState.h"
#include "ShooterTypes/ShooterTypes.h"
#include "Engine/World.h"


UEliminationComponent::UEliminationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	SequentialElimInterval = 2.f;
	LastElimTime = 0.f;
	SequentialElims = 0.f;

}

void UEliminationComponent::OnRoundReported(AActor* Attacker, AActor* Victim, bool bHit, bool bHeadShot, bool bLethal)
{
	AShooterPlayerState* AttackerPS = GetPlayerStateFromActor(Attacker);
	if (!IsValid(AttackerPS)) return;
	
	ProcessHitOrMiss(bHit, AttackerPS);
	
	if (!bHit) return;
	
	AShooterPlayerState* VictimPS = GetPlayerStateFromActor(Victim);
	if (!IsValid(VictimPS)) return;
	
	if (bLethal)
	{
		ProcessElimination(bHeadShot, AttackerPS, VictimPS);
	}
}

void UEliminationComponent::ProcessElimination(bool bHeadShot, AShooterPlayerState* AttackerPS,
	AShooterPlayerState* VictimPS)
{
	AttackerPS->AddScoredElim();
	VictimPS->AddDefeat();
	
	ESpecialElimType SpecialElimType{};
	ProcessHeadshot(bHeadShot, SpecialElimType, AttackerPS);
	ProcessSequentialEliminations(SpecialElimType, AttackerPS);
	// headshots and more
}

void UEliminationComponent::ProcessHeadshot(bool bHeadShot, ESpecialElimType& OutElimType,
	AShooterPlayerState* AttackerPS)
{
	if (bHeadShot)
	{
		OutElimType |= ESpecialElimType::HeadShot;
		AttackerPS->AddHeadshotElim();
	}
}

void UEliminationComponent::ProcessSequentialEliminations(ESpecialElimType& OutElimType,
	AShooterPlayerState* AttackerPS)
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastElimTime <= SequentialElimInterval)
	{
		++SequentialElims;
	}
	else
	{
		SequentialElims = 1;
	}
	
	LastElimTime = CurrentTime;
	
	if (SequentialElims > 1)
	{
		OutElimType |= ESpecialElimType::Sequential;
		AttackerPS->AddSequentialElim(SequentialElims);
	}
}

void UEliminationComponent::ProcessHitOrMiss(bool bHit, AShooterPlayerState* AttackerPS)
{
	if (bHit)
	{
		AttackerPS->AddHit();
	}
	else
	{
		AttackerPS->AddMiss();
	}
}

AShooterPlayerState* UEliminationComponent::GetPlayerStateFromActor(AActor* Actor)
{
	APawn* Pawn = Cast<APawn>(Actor);
	if (IsValid(Pawn))
	{
		return Pawn->GetPlayerState<AShooterPlayerState>();
	}
	return nullptr;
}


