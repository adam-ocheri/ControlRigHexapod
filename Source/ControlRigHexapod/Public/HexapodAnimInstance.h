// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HexapodAnimInstance.generated.h"

/**
 * 
 */
UCLASS(transient, Blueprintable)
class CONTROLRIGHEXAPOD_API UHexapodAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:

	UHexapodAnimInstance(const FObjectInitializer& ObjectInitializer);

	void BlueprintInitializeAnimation();

	void BlueprintUpdateAnimation(float DeltaTimeX);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod Transformations")
	FVector OwnerVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod Transformations")
	FRotator OwnerRotation;
};
