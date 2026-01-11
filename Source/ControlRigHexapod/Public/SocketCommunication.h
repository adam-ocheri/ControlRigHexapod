// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/Tickable.h"
#include "Common/TcpSocketBuilder.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "SocketSubsystem.h"
#include "SocketCommunication.generated.h"

/**
 * TCP Server that receives velocity and rotation data from a remote socket client (LOCAL)
 */
UCLASS()
class CONTROLRIGHEXAPOD_API USocketCommunication : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
	
public:
	USocketCommunication();
	virtual ~USocketCommunication();

	static USocketCommunication* CreateNewSocketCommunication(UObject* OwningAnimInstance);

	/** Port number to listen on */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket Settings")
	int32 ListenPort = 8888;

	/** Whether to automatically start the server on initialization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket Settings")
	bool bAutoStart = true;

	/** The most recently received velocity data */
	UPROPERTY(BlueprintReadOnly, Category = "Socket Data")
	FVector IncomingVelocity;

	/** The most recently received rotation data */
	UPROPERTY(BlueprintReadOnly, Category = "Socket Data")
	FRotator IncomingRotation;

	/** Start the TCP server */
	UFUNCTION(BlueprintCallable, Category = "Socket")
	bool StartServer();

	/** Stop the TCP server */
	UFUNCTION(BlueprintCallable, Category = "Socket")
	void StopServer();

	/** Check if the server is currently listening */
	UFUNCTION(BlueprintPure, Category = "Socket")
	bool IsServerRunning() const { return bIsListening; }

	/** Check if a client is currently connected */
	UFUNCTION(BlueprintPure, Category = "Socket")
	bool IsClientConnected() const { return ClientSocket != nullptr && ClientSocket->GetConnectionState() == SCS_Connected; }

	// FTickableGameObject interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

protected:
	/** The listening socket */
	FSocket* ListenSocket;

	/** The connected client socket */
	FSocket* ClientSocket;

	/** Socket subsystem for creating sockets */
	ISocketSubsystem* SocketSubsystem;

	/** Whether the server is currently listening */
	bool bIsListening;

	/** Check for new connections and incoming data */
	void TickConnection();

	/** Accept a new client connection */
	void AcceptConnection();

	/** Receive and process data from the client */
	void ReceiveData();

	/** Parse received binary data into velocity and rotation */
	void ParseReceivedData(const TArray<uint8>& Data);
};
