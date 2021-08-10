// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "GhoulCharacter.generated.h"

UCLASS()
class STRATEGYGAME_API AGhoulCharacter : public ABaseCharacter
{
	GENERATED_UCLASS_BODY()

		UFUNCTION(BlueprintCallable)
		float GetSpeed() const;

};
