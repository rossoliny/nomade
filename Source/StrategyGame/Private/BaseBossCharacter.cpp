// Fill out your copyright notice in the Description page of Project Settings.

#include "StrategyGame.h"
#include "BaseBossCharacter.h"

#include "StrategyAIController.h"

// Sets default values
ABaseBossCharacter::ABaseBossCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))

{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = AStrategyAIController::StaticClass();
}
