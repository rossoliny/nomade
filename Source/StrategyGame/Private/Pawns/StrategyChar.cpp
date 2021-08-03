// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "StrategyGame.h"
#include "StrategyAIController.h"
#include "StrategyAttachment.h"

AStrategyChar::AStrategyChar(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCharacterMovementComponent>(ACharacter::CharacterMovementComponentName)) 
{
	PrimaryActorTick.bCanEverTick = true;

	// no collisions in mesh
	GetMesh()->BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->UpdatedComponent = GetCapsuleComponent();
		GetCharacterMovement()->GetNavAgentPropertiesRef().bCanJump = true;
		GetCharacterMovement()->GetNavAgentPropertiesRef().bCanWalk = true;
		GetCharacterMovement()->SetJumpAllowed(true);
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->bAlwaysCheckFloor = false;
	}

	Health = 100.f;
}

void AStrategyChar::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// initialization
	//UpdatePawnData();
	TickHealth();
}

bool AStrategyChar::CanBeBaseForCharacter(APawn* Pawn) const
{
	return false;
}

void AStrategyChar::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalForce, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalForce, Hit);

	if (bSelfMoved)
	{
		// if we controlled by an AI, let it know we ran into something
		AStrategyAIController* const AI = Cast<AStrategyAIController>(Controller);
		if (AI)
		{
			AI->NotifyBump(Hit);
		}
	}
}



void AStrategyChar::FellOutOfWorld(const UDamageType& DamageType)
{
	// if we fall out of the world, die
	Die(Health, FDamageEvent(DamageType.GetClass()), nullptr, nullptr);
}

void AStrategyChar::SetWeaponAttachment(UStrategyAttachment* Weapon)
{
	if (WeaponSlot != Weapon)
	{
		// detach any existing weapon attachment
		if (WeaponSlot )
		{
			WeaponSlot->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		}

		// attach this one
		WeaponSlot = Weapon;
		if (WeaponSlot )
		{
			WeaponSlot->RegisterComponent();
			WeaponSlot->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, WeaponSlot->AttachPoint);
			UpdatePawnData();
			TickHealth();
		}
	}
}

void AStrategyChar::SetArmorAttachment(UStrategyAttachment* Armor)
{
	if (ArmorSlot != Armor)
	{
		// detach any existing armor attachment
		if (ArmorSlot )
		{
			ArmorSlot->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		}
		// attach this one
		ArmorSlot = Armor;
		if (ArmorSlot )
		{
			ArmorSlot->RegisterComponent();
			ArmorSlot->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, ArmorSlot->AttachPoint);
			UpdatePawnData();
			TickHealth();
		}
	}
}

bool AStrategyChar::IsWeaponAttached()
{
	return WeaponSlot != nullptr;
}

bool AStrategyChar::IsArmorAttached()
{
	return ArmorSlot != nullptr;
}


void AStrategyChar::ApplyBuff(const FBuffData& Buff)
{
	// calc the end time
	FBuffData NewBuff = Buff;
	if (!Buff.bInfiniteDuration)
	{
		NewBuff.EndTime = GetWorld()->GetTimeSeconds() + Buff.Duration;
	}

	// add to active buffs
	ActiveBuffs.Add(NewBuff);

	// update to account for changes
	UpdatePawnData();
	TickHealth();
}

int32 AStrategyChar::GetMaxHealth() const
{
	return GetClass()->GetDefaultObject<AStrategyChar>()->GetHealth() + PawnData.MaxHealthBonus;
}

void FBuffData::ApplyBuff(struct FPawnData& PawnData)
{
	PawnData.AttackMin += BuffData.AttackMin;
	PawnData.AttackMax += BuffData.AttackMax;
	PawnData.DamageReduction += BuffData.DamageReduction;
	PawnData.HealthRegen += BuffData.HealthRegen;
	PawnData.MaxHealthBonus += BuffData.MaxHealthBonus;
	PawnData.Speed += BuffData.Speed;
}

void AStrategyChar::UpdatePawnData()
{
	Super::UpdatePawnData();
	// add influence of any attachments
	UStrategyAttachment* const InvSlots[] = { WeaponSlot, ArmorSlot };
	for (int32 i = 0; i < UE_ARRAY_COUNT(InvSlots); i++)
	{
		if (InvSlots[i])
		{
			InvSlots[i]->Effect.ApplyBuff(PawnData);
		}
	}
}

void AStrategyChar::TickHealth()
{
	if ( (Health > 0.f) && (PawnData.HealthRegen != 0.f) )
	{
		if (PawnData.HealthRegen < 0.f)
		{
			// negative health regen is a DoT, so do the damage
			UGameplayStatics::ApplyDamage(this, -PawnData.HealthRegen, Controller, this, UDamageType::StaticClass());
		}
		else
		{
			// add health regen
			Health = FMath::Min<int32>(Health + PawnData.HealthRegen, GetMaxHealth());
		}
	}

	// update again in 1 second
	GetWorldTimerManager().SetTimer(TimerHandle_UpdateHealth, this, &AStrategyChar::TickHealth, 1.0f, false);
}
