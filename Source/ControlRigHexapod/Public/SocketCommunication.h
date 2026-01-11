// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SocketCommunication.generated.h"

/**
 * 
 */
UCLASS()
class CONTROLRIGHEXAPOD_API USocketCommunication : public UObject
{
	GENERATED_BODY()
	
public:

	static USocketCommunication* CreateNewSocketCommunication(UObject* OwningAnimInstance);

	FVector IncomingVelocity;

	FRotator IncomingRotation;
};
