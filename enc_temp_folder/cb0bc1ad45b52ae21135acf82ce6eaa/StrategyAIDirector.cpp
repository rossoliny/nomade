// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "StrategyGame.h"
#include "StrategyAIDirector.h"

#include "DevUtils.h"
#include "StrategyBuilding_Brewery.h"
#include "StrategyGameBlueprintLibrary.h"
#include "StrategyAttachment.h"
#include "LycanthropeCharacter.h"
#include "GhoulCharacter.h"
#include "ZombieCharacter.h"

UStrategyAIDirector::UStrategyAIDirector(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WaveSize(3)
	, RadiusToSpawnOn(200)
	, CustomScale(1.0)
	, AnimationRate(1)
	, NextDwarfSpawnTime(0)
	, NextZombieSpawnTime(0)
	, MyTeamNum(EStrategyTeam::Unknown)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// @todo, why aren't these set in BuffData ctor?
	BuffModifier.BuffData.AttackMin = 0;
	BuffModifier.BuffData.AttackMax = 0;
	BuffModifier.BuffData.DamageReduction = 0;
	BuffModifier.BuffData.MaxHealthBonus = 0;
	BuffModifier.BuffData.HealthRegen = 0;
	BuffModifier.BuffData.Speed = 0;
	BuffModifier.Duration = 0;
	BuffModifier.bInfiniteDuration = false;
}

uint8 UStrategyAIDirector::GetTeamNum() const
{
	return MyTeamNum;
}

void  UStrategyAIDirector::SetTeamNum(uint8 inTeamNum)
{
	MyTeamNum = inTeamNum;
}

void UStrategyAIDirector::OnGameplayStateChange(EGameplayState::Type NewState)
{
	if (NewState == EGameplayState::Playing)
	{
		Activate();
		NextDwarfSpawnTime = 0;
	}
}

AStrategyBuilding_Brewery* UStrategyAIDirector::GetEnemyBrewery() const
{
	return EnemyBrewery.Get();
}

void UStrategyAIDirector::SetDefaultArmor(UBlueprint* InArmor)
{
	DefaultArmor = InArmor ? *InArmor->GeneratedClass : nullptr;
}

void UStrategyAIDirector::SetDefaultArmorClass(TSubclassOf<UStrategyAttachment> InArmor)
{
	DefaultArmor = InArmor;
}

void UStrategyAIDirector::SetDefaultWeapon(UBlueprint* InWeapon)
{
	DefaultWeapon = InWeapon ? *InWeapon->GeneratedClass : nullptr;
}

void UStrategyAIDirector::SetDefaultWeaponClass(TSubclassOf<UStrategyAttachment> InWeapon)
{
	DefaultWeapon = InWeapon;
}

void UStrategyAIDirector::SetBuffModifier(AStrategyChar* InChar, int32 AttackMin, int32 AttackMax, int32 DamageReduction, int32 MaxHealthBonus, int32 HealthRegen, float Speed, int32 DrunkLevel, float Duration, bool bInfiniteDuration, float InCustomScale, float InAnimaRate)
{
	BuffModifier.BuffData.AttackMin = AttackMin;
	BuffModifier.BuffData.AttackMax = AttackMax;
	BuffModifier.BuffData.DamageReduction = DamageReduction;
	BuffModifier.BuffData.MaxHealthBonus = MaxHealthBonus;
	BuffModifier.BuffData.HealthRegen = HealthRegen;
	BuffModifier.BuffData.Speed = Speed;
	BuffModifier.Duration = Duration;
	BuffModifier.bInfiniteDuration = bInfiniteDuration;
	CustomScale = InCustomScale;
	AnimationRate = InAnimaRate;
}

struct OffsetsGeneratorHelper
{
	float Offset[6];
	int32 LastIndex;

	OffsetsGeneratorHelper()
		: LastIndex( FMath::RandRange(0, 5) )
	{
		TArray<float> AllSlots;
		for (int32 Idx = 0; Idx < 6; Idx++)
		{
			AllSlots.Add((Idx - 6/2) * 45);
		}

		// let's give better order for our spots
		const int32 Indexes[6] = {3,2,4,1,5,0};
		for(int32 Idx = 0; Idx < 6; Idx++)
		{
			Offset[Idx] = AllSlots[Indexes[Idx]];
		}
	}

	float GetOffset()
	{
		LastIndex = ++LastIndex >= 6 ? 0 : LastIndex;
		return Offset[LastIndex];
	}
};

#pragma optimize("", off)
void UStrategyAIDirector::SpawnDwarfs()
{
	static OffsetsGeneratorHelper OffsetsGenerator;

	const bool bShoudSpawnNewUnits = GetWorld()->GetTimeSeconds() > NextDwarfSpawnTime;
	if (!bShoudSpawnNewUnits)
	{
		return;
	}

	if (EnemyBrewery == nullptr)
	{
		const EStrategyTeam::Type EnemyTeamNum = (MyTeamNum == EStrategyTeam::Player ? EStrategyTeam::Enemy : EStrategyTeam::Player);
		const FPlayerData* const EnemyTeamData = GetWorld()->GetGameState<AStrategyGameState>()->GetPlayerData(EnemyTeamNum);
		if (EnemyTeamData != nullptr && EnemyTeamData->Brewery != nullptr)
		{
			EnemyBrewery = EnemyTeamData->Brewery;
		}
	}

	if(WaveSize > 0)
	{
		// find best place on ground to spawn at
		const AStrategyBuilding_Brewery* const Owner = Cast<AStrategyBuilding_Brewery>(GetOwner());
		check(Owner);
		bool bSpawnedNewMinion = false;
		if( Owner->DwarfCharClass != nullptr )
		{
			FVector Loc = Owner->GetActorLocation();
			const FVector X = Owner->GetTransform().GetScaledAxis( EAxis::X );
			const FVector Y = Owner->GetTransform().GetScaledAxis( EAxis::Y );
			Loc += X * RadiusToSpawnOn +  Y * OffsetsGenerator.GetOffset();

			const FVector Scale(CustomScale);
			const FVector TraceOffset(0.0f,0.0f,RadiusToSpawnOn * 0.5 * Scale.Z);
			FHitResult Hit;
			FCollisionObjectQueryParams ObjectParams( FCollisionObjectQueryParams::AllStaticObjects );
			GetWorld()->LineTraceSingleByObjectType(Hit, Loc + TraceOffset, Loc - TraceOffset, ObjectParams);
			if (Hit.Actor.IsValid())
			{
				Loc = Hit.Location + FVector(0.0f,0.0f,Scale.Z * 10.0f);
			}
			AStrategyChar* StrategyChar = Owner->DwarfCharClass->GetDefaultObject<AStrategyChar>();
			const float CapsuleHalfHeight = StrategyChar->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
			const float CapsuleRadius = StrategyChar->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
			Loc = Loc + FVector( 0.0f,0.0f,Scale.Z * CapsuleHalfHeight);

			// and spawn our minion
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			auto rot = Owner->GetActorRotation();
			doNotOptimizeAway(rot);
			AStrategyChar* const MinionChar =  GetWorld()->SpawnActor<AStrategyChar>(Owner->DwarfCharClass, Loc, rot, SpawnInfo);
			// don't continue if he died right away on spawn
			if ( (MinionChar != nullptr) && (MinionChar->bIsDying == false) )
			{
				// Flag a successful spawn
				bSpawnedNewMinion = true;

				MinionChar->SetTeamNum(GetTeamNum());

				MinionChar->SpawnDefaultController();
				MinionChar->GetCapsuleComponent()->SetRelativeScale3D(Scale);
				MinionChar->GetCapsuleComponent()->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
				MinionChar->GetMesh()->GlobalAnimRateScale = AnimationRate;

				AStrategyGameState* const GameState = GetWorld()->GetGameState<AStrategyGameState>();
				if (GameState != nullptr)
				{
					GameState->OnCharSpawned(MinionChar);
				}

				MinionChar->ApplyBuff(BuffModifier);
				if (DefaultWeapon != nullptr)
				{
					UStrategyGameBlueprintLibrary::GiveWeaponFromClass(MinionChar, DefaultWeapon);
				}
				if (DefaultArmor != nullptr)
				{
					UStrategyGameBlueprintLibrary::GiveArmorFromClass(MinionChar, DefaultArmor);
				}

				WaveSize -= 1;
				WaveSize = FMath::Max(WaveSize, 0);
				if (Owner != nullptr && WaveSize <= 0 && MyTeamNum==EStrategyTeam::Enemy)
				{
					Owner->OnWaveSpawned.Broadcast();
				}
				NextDwarfSpawnTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(2.0f, 3.0f);
			}
			else
			{
				UE_LOG(LogGame, Warning, TEXT("Failed to spawn minion.") );
			}
		}
		else
		{
			// If we dont have a class type we cannot spawn a minion. 
			UE_LOG(LogGame, Warning, TEXT("No minion class specified in %s. Cannot spawn minion"), *Owner->GetName() );			
		}
		// If we failed to spawn a minion try again soon
		if( bSpawnedNewMinion == false )
		{
			NextDwarfSpawnTime = GetWorld()->GetTimeSeconds() + 0.1f;
		}
	}
}

void UStrategyAIDirector::SpawnZombies()
{
	static OffsetsGeneratorHelper OffsetsGenerator;

	const bool bShoudSpawnNewUnits = GetWorld()->GetTimeSeconds() > NextZombieSpawnTime && MyTeamNum == EStrategyTeam::Enemy;
	if (!bShoudSpawnNewUnits)
	{
		return;
	}
	const AStrategyBuilding_Brewery* const Owner = Cast<AStrategyBuilding_Brewery>(GetOwner());
	
	FVector Loc = Owner->GetActorLocation();
	const FVector X = Owner->GetTransform().GetScaledAxis(EAxis::X);
	const FVector Y = Owner->GetTransform().GetScaledAxis(EAxis::Y);
	Loc += X * RadiusToSpawnOn + Y * OffsetsGenerator.GetOffset();

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto rot = Owner->GetActorRotation();
	doNotOptimizeAway(rot);
	
	AZombieCharacter* const ZombieChar = GetWorld()->SpawnActor<AZombieCharacter>(Owner->ZombieCharClass, Loc, rot, SpawnInfo);

	if ((ZombieChar != nullptr))
	{
		// Flag a successful spawn

		ZombieChar->SetTeamNum(GetTeamNum());

		ZombieChar->SpawnDefaultController();
		//ZombieChar->GetCapsuleComponent()->SetRelativeScale3D(Scale);
		//ZombieChar->GetCapsuleComponent()->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
		//ZombieChar->GetMesh()->GlobalAnimRateScale = AnimationRate;

		AStrategyGameState* const GameState = GetWorld()->GetGameState<AStrategyGameState>();
		if (GameState != nullptr)
		{
			GameState->OnCharSpawned(ZombieChar);
		}
		/*
//		ZombieChar->ApplyBuff(BuffModifier);
		ZombieWaveSize -= 1;
		ZombieWaveSize = FMath::Max(WaveSize, 0);
		if (Owner != nullptr && ZombieWaveSize <= 0 && MyTeamNum == EStrategyTeam::Enemy)
		{
			//Owner->OnWaveSpawned.Broadcast();
		}
		*/
		NextZombieSpawnTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(6.0f, 7.0f);
		
	}
}



void UStrategyAIDirector::SpawnGhouls()
{
	static OffsetsGeneratorHelper OffsetsGenerator;

	const bool bShoudSpawnNewUnits = GetWorld()->GetTimeSeconds() > NextGhoulSpawnTime && MyTeamNum == EStrategyTeam::Enemy;
	if (!bShoudSpawnNewUnits)
	{
		return;
	}
	const AStrategyBuilding_Brewery* const Owner = Cast<AStrategyBuilding_Brewery>(GetOwner());

	FVector Loc = Owner->GetActorLocation();
	const FVector X = Owner->GetTransform().GetScaledAxis(EAxis::X);
	const FVector Y = Owner->GetTransform().GetScaledAxis(EAxis::Y);
	Loc += X * RadiusToSpawnOn + Y * OffsetsGenerator.GetOffset();

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto rot = Owner->GetActorRotation();
	doNotOptimizeAway(rot);

	AGhoulCharacter* const  GhoulChar = GetWorld()->SpawnActor<AGhoulCharacter>(Owner->GhoulCharClass, Loc, rot, SpawnInfo);

	if ((GhoulChar != nullptr))
	{
		
		GhoulChar->SetTeamNum(GetTeamNum());

		GhoulChar->SpawnDefaultController();
		AStrategyGameState* const GameState = GetWorld()->GetGameState<AStrategyGameState>();
		if (GameState != nullptr)
		{
			GameState->OnCharSpawned(GhoulChar);
		}

		NextGhoulSpawnTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(6.0f, 7.0f);

	}
}




void UStrategyAIDirector::SpawnLycanthrope()
{
	static OffsetsGeneratorHelper OffsetsGenerator;

	const bool bShoudSpawnNewUnits = GetWorld()->GetTimeSeconds() > NextLycanthropeSpawnTime && MyTeamNum == EStrategyTeam::Enemy;
	if (!bShoudSpawnNewUnits)
	{
		return;
	}
	const AStrategyBuilding_Brewery* const Owner = Cast<AStrategyBuilding_Brewery>(GetOwner());

	FVector Loc = Owner->GetActorLocation();
	const FVector X = Owner->GetTransform().GetScaledAxis(EAxis::X);
	const FVector Y = Owner->GetTransform().GetScaledAxis(EAxis::Y);
	Loc += X * RadiusToSpawnOn + Y * OffsetsGenerator.GetOffset();

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto rot = Owner->GetActorRotation();
	doNotOptimizeAway(rot);

	ALycanthropeCharacter* const  LycanthropelChar = GetWorld()->SpawnActor<ALycanthropeCharacter>(Owner->LycanthropeClass, Loc, rot, SpawnInfo);

	if ((LycanthropelChar != nullptr))
	{
		LycanthropelChar->SetTeamNum(GetTeamNum());

		LycanthropelChar->SpawnDefaultController();
		AStrategyGameState* const GameState = GetWorld()->GetGameState<AStrategyGameState>();
		if (GameState != nullptr)
		{
			GameState->OnCharSpawned(LycanthropelChar);
		}

		NextLycanthropeSpawnTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(6.0f, 7.0f);

	}
}




#pragma optimize("", on)

void UStrategyAIDirector::RequestSpawn()
{
	WaveSize += 1;
}

void UStrategyAIDirector::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SpawnDwarfs();
	SpawnZombies();
	SpawnGhouls();
	SpawnLycanthrope();
}