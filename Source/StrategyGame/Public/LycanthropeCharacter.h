// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseBossCharacter.h"
#include "LycanthropeCharacter.generated.h"

UCLASS()
class STRATEGYGAME_API ALycanthropeCharacter : public ABaseBossCharacter
{
	GENERATED_UCLASS_BODY()

		UFUNCTION(BlueprintCallable)
		float GetSpeed() const;

};
