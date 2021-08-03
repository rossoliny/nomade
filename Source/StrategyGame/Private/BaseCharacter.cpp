// Fill out your copyright notice in the Description page of Project Settings.

#include "StrategyGame.h"
#include "BaseCharacter.h"

#include "StrategyAIController.h"

// Sets default values
ABaseCharacter::ABaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
	, ResourcesToGather(10)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = AStrategyAIController::StaticClass();
}

void ABaseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	UpdatePawnData();
}

// Called when the game starts or when spawned
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

float ABaseCharacter::PlayAttackAnim()
{
	if ((Health > 0.f) && AttackAnim)
	{
		return PlayAnimMontage(AttackAnim);
	}

	return 0.f;
}

void ABaseCharacter::UpdatePawnData()
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	float TimeToNextUpdate = -1.f;

	// start from existing base data
	FPawnData NewPawnData = DefaultPawnData;

	// add in influence of any active buffs
	for (int32 i = 0; i < ActiveBuffs.Num(); i++)
	{
		// expired buff, clear it out
		if (!ActiveBuffs[i].bInfiniteDuration && CurrentTime >= ActiveBuffs[i].EndTime)
		{
			ActiveBuffs.RemoveAt(i);
			i--;
			continue;
		}

		if (TimeToNextUpdate < 0 || TimeToNextUpdate > ActiveBuffs[i].EndTime - CurrentTime)
		{
			TimeToNextUpdate = ActiveBuffs[i].EndTime - CurrentTime;
		}

		ActiveBuffs[i].ApplyBuff(NewPawnData);
	}

	// validate some of our data; only health regen can have negative values
	NewPawnData.AttackMin = FMath::Max(0, NewPawnData.AttackMin);
	NewPawnData.AttackMax = FMath::Max(0, NewPawnData.AttackMax);
	NewPawnData.DamageReduction = FMath::Max(0, NewPawnData.DamageReduction);
	NewPawnData.MaxHealthBonus = FMath::Max(0, NewPawnData.MaxHealthBonus);

	// store the final values
	PawnData = NewPawnData;

	// make sure new health doesn't exceed the cap
	Health = FMath::Min<int32>(Health + PawnData.MaxHealthBonus, GetMaxHealth());

	// update groundspeed
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.0f, GetClass()->GetDefaultObject<ABaseCharacter>()->GetCharacterMovement()->MaxWalkSpeed + NewPawnData.Speed);
	}

	// update the buffs next time any expires, they are also updated when any buff is added
	if (TimeToNextUpdate > 0.f)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_UpdatePawnData, this, &ABaseCharacter::UpdatePawnData, TimeToNextUpdate, false);
	}

}

// Called every frame
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

uint8 ABaseCharacter::GetTeamNum() const
{
	return MyTeamNum;
}

void ABaseCharacter::SetTeamNum(uint8 NewTeamNum)
{
	MyTeamNum = NewTeamNum;
}


int32 ABaseCharacter::GetHealth() const
{
	return Health;
}

int32 ABaseCharacter::GetMaxHealth() const
{
	return GetClass()->GetDefaultObject<ABaseCharacter>()->GetHealth();
}


void ABaseCharacter::OnMeleeAttackFinishedNotify()
{
	const int32 MeleeDamage = FMath::RandRange(PawnData.AttackMin, PawnData.AttackMax);
	const TSubclassOf<UDamageType> MeleeDmgType = UDamageType::StaticClass();

	// Do a trace to see what we hit
	const float CollisionRadius = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.f;
	const float TraceDistance = CollisionRadius + (PawnData.AttackDistance * 1.3f);
	const FVector TraceStart = GetActorLocation();
	const FVector TraceDir = GetActorForwardVector();
	const FVector TraceEnd = TraceStart + TraceDir * TraceDistance;
	TArray<FHitResult> Hits;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(MeleeHit), false, this);
	FCollisionResponseParams ResponseParam(ECollisionResponse::ECR_Overlap);
	GetWorld()->SweepMultiByChannel(Hits, TraceStart, TraceEnd, FQuat::Identity, COLLISION_WEAPON, FCollisionShape::MakeBox(FVector(80.f)), TraceParams, ResponseParam);

	for (int32 i = 0; i < Hits.Num(); i++)
	{
		FHitResult const& Hit = Hits[i];
		if (AStrategyGameMode::OnEnemyTeam(this, Hit.GetActor()))
		{
			UGameplayStatics::ApplyPointDamage(Hit.GetActor(), MeleeDamage, TraceDir, Hit, Controller, this, MeleeDmgType);

			// only damage first hit
			break;
		}
	}
}


float ABaseCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Health <= 0.f)
	{
		// no further damage if already dead
		return 0.f;
	}

	// Modify based on game rules.
	AStrategyGameMode* const Game = GetWorld()->GetAuthGameMode<AStrategyGameMode>();
	Damage = Game ? Game->ModifyDamage(Damage, this, DamageEvent, EventInstigator, DamageCauser) : 0.f;

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage > 0.f)
	{
		Health -= ActualDamage;
		if (Health <= 0.f)
		{
			Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
		}

		// broadcast AI-detectable noise
		MakeNoise(1.0f, EventInstigator ? EventInstigator->GetPawn() : this);

		// our gamestate wants to know when damage happens
		AStrategyGameState* const GameState = GetWorld()->GetGameState<AStrategyGameState>();
		if (GameState)
		{
			GameState->OnActorDamaged(this, ActualDamage, EventInstigator);
		}
	}

	return ActualDamage;
}


void ABaseCharacter::Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser)
{
	if (bIsDying										// already dying
		|| IsPendingKill())								// already destroyed
	{
		return;
	}

	bIsDying = true;
	Health = FMath::Min(0.0f, Health);

	// figure out who killed us
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ?
		Cast<const UDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject())
		: GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	// forcibly end any timers that may be in flight
	GetWorldTimerManager().ClearAllTimersForObject(this);

	// notify the game mode if an Enemy dies
	if (GetTeamNum() == EStrategyTeam::Enemy)
	{
		AStrategyGameState* const GameState = GetWorld()->GetGameState<AStrategyGameState>();
		if (GameState)
		{
			GameState->OnCharDied(this);
		}
	}

	// disable any AI
	AStrategyAIController* const AIController = Cast<AStrategyAIController>(Controller);
	if (AIController)
	{
		AIController->EnableLogic(false);
	}

	// turn off collision
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCapsuleComponent()->BodyInstance.SetResponseToChannel(ECC_Pawn, ECR_Ignore);
		GetCapsuleComponent()->BodyInstance.SetResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	}

	// turn off movement
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}

	// detach the controller
	if (Controller != nullptr)
	{
		Controller->UnPossess();
	}

	// play death animation
	float DeathAnimDuration = 0.f;
	if (DeathAnim)
	{
		DeathAnimDuration = PlayAnimMontage(DeathAnim) / (GetMesh() && GetMesh()->GlobalAnimRateScale > 0 ? GetMesh()->GlobalAnimRateScale : 1);
		UAnimInstance * AnimInstance = (GetMesh()) ? GetMesh()->GetAnimInstance() : nullptr;
	}

	// Use a local timer handle as we don't need to store it for later but we don't need to look for something to clear
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ABaseCharacter::OnDieAnimationEnd, DeathAnimDuration + 0.01, false);
}

void ABaseCharacter::OnDieAnimationEnd()
{
	this->SetActorHiddenInGame(true);
	// delete the pawn asap
	SetLifeSpan(0.01f);
}
