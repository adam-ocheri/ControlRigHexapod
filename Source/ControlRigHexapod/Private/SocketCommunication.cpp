// Fill out your copyright notice in the Description page of Project Settings.


#include "SocketCommunication.h"

USocketCommunication* USocketCommunication::CreateNewSocketCommunication(UObject* OwningAnimInstance)
{
    return NewObject< USocketCommunication>(OwningAnimInstance, FName("HexapodSocket"), RF_Public);
}
