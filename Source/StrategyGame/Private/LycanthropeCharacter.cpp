// Fill out your copyright notice in the Description page of Project Settings.


#include "StrategyGame.h"
#include "LycanthropeCharacter.h"

// Sets default values
ALycanthropeCharacter::ALycanthropeCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	GetMesh()->BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->UpdatedComponent = GetCapsuleComponent();
		GetCharacterMovement()->GetNavAgentPropertiesRef().bCanJump = true;
		GetCharacterMovement()->GetNavAgentPropertiesRef().bCanWalk = true;
		GetCharacterMovement()->SetJumpAllowed(true);
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->bAlwaysCheckFloor = false;
		GetCharacterMovement()->MaxWalkSpeed = 0;
	}

	Health = 100.f;

}


float ALycanthropeCharacter::GetSpeed() const
{
	return PawnData.Speed;
}


