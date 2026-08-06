// Copyright Isaac Turpin


#include "Combat/CombatComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "Weapon/Weapon.h"


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}

void UCombatComponent::Initiate_CycleWeapon()
{
}

void UCombatComponent::Initiate_FireWeapon_Pressed()
{
}

void UCombatComponent::Initiate_FireWeapon_Released()
{
}

void UCombatComponent::Initiate_ReloadWeapon()
{
}

void UCombatComponent::Initiate_Aim_Pressed()
{
}

void UCombatComponent::Initiate_Aim_Released()
{
}

void UCombatComponent::SpawnInventory()
{
	AWeapon* NewWeapon = SpawnWeapon(DefaultWeaponClass);
	if (IsValid(NewWeapon))
	{
		NewWeapon->AttachToOwningPawn();
	}
}

void UCombatComponent::DestroyInventory()
{
	// TODO: Destroy inventory
}

AWeapon* UCombatComponent::SpawnWeapon(TSubclassOf<AWeapon> WeaponClass) const
{
	AActor* OwningActor = GetOwner();
	if (!IsValid(OwningActor)) return nullptr;
	if (OwningActor->GetLocalRole() < ROLE_Authority) return nullptr;
	
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = Cast<APawn>(OwningActor);
	SpawnInfo.Owner = OwningActor;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	return GetWorld()->SpawnActor<AWeapon>(WeaponClass, SpawnInfo);
}

