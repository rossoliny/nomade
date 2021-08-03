// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "ZombieCharacter.generated.h"

/**
 * 
 */
UCLASS()
class STRATEGYGAME_API AZombieCharacter : public ABaseCharacter
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable)
	float GetSpeed() const;
};
