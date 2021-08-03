// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseCharacter.h"
#include "StrategyTypes.h"
#include "StrategyTeamInterface.h"
#include "StrategyChar.generated.h"


class UStrategyAttachment;

// Base class for the minions
UCLASS(Abstract)
class AStrategyChar : public ABaseCharacter
{
	GENERATED_UCLASS_BODY()

public:


	// Notification that we have fallen out of the world.
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	// Begin Actor interface
	/** initial setup */
	virtual void PostInitializeComponents() override;

	/** prevent units from basing on each other or buildings */
	virtual bool CanBeBaseForCharacter(APawn* Pawn) const override;

	/** don't export collisions for navigation */
	virtual bool IsComponentRelevantForNavigation(UActorComponent* Component) const override { return false; }

	/** pass hit notifies to AI */
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalForce, const FHitResult& Hit) override;
	// End Actor interface


	/** set attachment for weapon slot */
	UFUNCTION(BlueprintCallable, Category=Attachment)
	void SetWeaponAttachment(UStrategyAttachment* Weapon);

	UFUNCTION(BlueprintCallable, Category=Attachment)
	bool IsWeaponAttached();

	/** set attachment for armor slot */
	UFUNCTION(BlueprintCallable, Category=Attachment)
	void SetArmorAttachment(UStrategyAttachment* Armor);

	UFUNCTION(BlueprintCallable, Category=Attachment)
	bool IsArmorAttached();

	/** adds active buff to this pawn */
	void ApplyBuff(const struct FBuffData& Buff);

	virtual int32 GetMaxHealth() const override;

protected:
	/** Armor attachment slot */
	UPROPERTY()
	UStrategyAttachment* ArmorSlot;

	/** Weapon attachment slot */
	UPROPERTY()
	UStrategyAttachment* WeaponSlot;

	/** update pawn data after changes in active buffs */
	virtual void UpdatePawnData() override;

	/** update pawn's health */
	void TickHealth();

private:

	/** Handle for efficient management of TickHealth timer */
	FTimerHandle TimerHandle_UpdateHealth;
};

