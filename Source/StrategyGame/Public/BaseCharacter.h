// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "StrategyTeamInterface.h"
#include "StrategyTypes.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

UCLASS()
class STRATEGYGAME_API ABaseCharacter : public ACharacter, public IStrategyTeamInterface

{
	GENERATED_UCLASS_BODY()
	
	/** How many resources this pawn is worth when it dies. */
	UPROPERTY(EditAnywhere, Category=Pawn)
	int32 ResourcesToGather;

	/** Current health of this Pawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Health)
	float Health;

	const FPawnData* GetDefaultPawnData() const { return &DefaultPawnData; }

	/** get all modifiers we have now on pawn */
	const FPawnData& GetModifiedPawnData() const  { return PawnData; }

	virtual void PostInitializeComponents() override;
	
	/** Identifies if pawn is in its dying state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Health)
	uint32 bIsDying : 1;
	
protected:
	virtual void BeginPlay() override;

	/** melee anim */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* AttackAnim;

	/** death anim */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* DeathAnim;
	
	/** team number */
	uint8 MyTeamNum;

	/** base pawn data */
	UPROPERTY(EditDefaultsOnly, Category = Pawn)
	FPawnData DefaultPawnData;

	/** pawn data with added buff effects */
	FPawnData PawnData;
	
	/** List of active buffs */
	TArray<struct FBuffData> ActiveBuffs;

	/** update pawn data after changes in active buffs */
	virtual void UpdatePawnData();

public:	

	/**
	 * Starts melee attack.
	 * @return Duration of the attack anim.
	 */
	virtual float PlayAttackAnim();
	virtual void Tick(float DeltaTime) override;
	
	/** Notification triggered from the melee animation to signal impact. */
	virtual void OnMeleeAttackFinishedNotify();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 * Kills pawn.
	 * @param KillingDamage - Damage amount of the killing blow
	 * @param DamageEvent - Damage event of the killing blow
	 * @param Killer - Who killed this pawn
	 * @param DamageCauser - the Actor that directly caused the damage (i.e. the Projectile that exploded, the Weapon that fired, etc)
	 * @returns true if allowed
	 */
	virtual void Die(float KillingDamage, struct FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser);

	/** Take damage, handle death */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual uint8 GetTeamNum() const override;
	virtual void SetTeamNum(uint8 NewTeamNum);

	/** get current health */
	UFUNCTION(BlueprintCallable, Category = Health)
	virtual int32 GetHealth() const;

	/** get max health */
	UFUNCTION(BlueprintCallable, Category = Health)
	virtual int32 GetMaxHealth() const;

	/** event called after die animation  to hide character and delete it asap */
	void OnDieAnimationEnd();
private:
	/** Handle for efficient management of UpdatePawnData timer */
	FTimerHandle TimerHandle_UpdatePawnData;
};
