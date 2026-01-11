// Fill out your copyright notice in the Description page of Project Settings.

#include "SocketCommunication.h"

USocketCommunication::USocketCommunication()
{
	ListenSocket = nullptr;
	ClientSocket = nullptr;
	SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	bIsListening = false;
	IncomingVelocity = FVector::ZeroVector;
	IncomingRotation = FRotator::ZeroRotator;
}

USocketCommunication::~USocketCommunication()
{
	StopServer();
}

USocketCommunication* USocketCommunication::CreateNewSocketCommunication(UObject* OwningAnimInstance)
{
	USocketCommunication* SocketComm = NewObject<USocketCommunication>(OwningAnimInstance, FName("HexapodSocket"), RF_Public);
	
	if (SocketComm && SocketComm->bAutoStart)
	{
		SocketComm->StartServer();
	}
	
	return SocketComm;
}

bool USocketCommunication::StartServer()
{
	if (bIsListening)
	{
		UE_LOG(LogTemp, Warning, TEXT("SocketCommunication: Server is already running"));
		return false;
	}

	if (SocketSubsystem == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("SocketCommunication: Failed to get socket subsystem"));
		return false;
	}

	// Create TCP listener socket
	ListenSocket = FTcpSocketBuilder(TEXT("SocketCommunicationServer"))
		.AsReusable()
		.BoundToPort(ListenPort)
		.Listening(8); // Maximum number of pending connections

	if (ListenSocket == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("SocketCommunication: Failed to create listener socket on port %d"), ListenPort);
		return false;
	}

	// Get the local address
	TSharedRef<FInternetAddr> LocalAddr = SocketSubsystem->CreateInternetAddr();
	ListenSocket->GetAddress(*LocalAddr);
	
	bIsListening = true;
	UE_LOG(LogTemp, Log, TEXT("SocketCommunication: Server started and listening on port %d"), ListenPort);

	return true;
}

void USocketCommunication::StopServer()
{
	// Close client connection
	if (ClientSocket != nullptr)
	{
		if (ClientSocket->GetConnectionState() == SCS_Connected)
		{
			ClientSocket->Close();
			UE_LOG(LogTemp, Log, TEXT("SocketCommunication: Client disconnected"));
		}
		
		if (SocketSubsystem != nullptr)
		{
			SocketSubsystem->DestroySocket(ClientSocket);
		}
		ClientSocket = nullptr;
	}

	// Close listener socket
	if (ListenSocket != nullptr)
	{
		if (bIsListening)
		{
			ListenSocket->Close();
			UE_LOG(LogTemp, Log, TEXT("SocketCommunication: Server stopped"));
		}
		
		if (SocketSubsystem != nullptr)
		{
			SocketSubsystem->DestroySocket(ListenSocket);
		}
		ListenSocket = nullptr;
	}

	bIsListening = false;
}

void USocketCommunication::Tick(float DeltaTime)
{
	TickConnection();
}

bool USocketCommunication::IsTickable() const
{
	// Only tick if the server is listening
	return bIsListening;
}

TStatId USocketCommunication::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USocketCommunication, STATGROUP_Tickables);
}

void USocketCommunication::TickConnection()
{
	if (!bIsListening)
	{
		return;
	}

	// If no client is connected, try to accept a new connection
	if (ClientSocket == nullptr || ClientSocket->GetConnectionState() != SCS_Connected)
	{
		AcceptConnection();
	}

	// If a client is connected, receive data
	if (ClientSocket != nullptr && ClientSocket->GetConnectionState() == SCS_Connected)
	{
		ReceiveData();
	}
}

void USocketCommunication::AcceptConnection()
{
	if (ListenSocket == nullptr || !bIsListening)
	{
		return;
	}

	bool bHasPendingConnection = false;
	if (ListenSocket->HasPendingConnection(bHasPendingConnection) && bHasPendingConnection)
	{
		TSharedRef<FInternetAddr> RemoteAddress = SocketSubsystem->CreateInternetAddr();
		ClientSocket = ListenSocket->Accept(*RemoteAddress, TEXT("SocketCommunicationClient"));

		if (ClientSocket != nullptr)
		{
			UE_LOG(LogTemp, Log, TEXT("SocketCommunication: Client connected from %s"), *RemoteAddress->ToString(true));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SocketCommunication: Failed to accept client connection"));
		}
	}
}

void USocketCommunication::ReceiveData()
{
	if (ClientSocket == nullptr || ClientSocket->GetConnectionState() != SCS_Connected)
	{
		return;
	}

	uint32 DataSize = 0;
	bool bHasData = ClientSocket->HasPendingData(DataSize);

	if (bHasData && DataSize > 0)
	{
		// We expect exactly 24 bytes (6 floats * 4 bytes each)
		// But we should handle partial receives
		TArray<uint8> ReceivedData;
		ReceivedData.SetNumUninitialized(DataSize);

		int32 BytesRead = 0;
		bool bRead = ClientSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead);

		if (bRead && BytesRead > 0)
		{
			// If we received exactly 24 bytes or more, parse it
			if (BytesRead >= 24)
			{
				// Take only the first 24 bytes (one complete message)
				TArray<uint8> CompleteMessage;
				CompleteMessage.Append(ReceivedData.GetData(), 24);
				ParseReceivedData(CompleteMessage);

				// If we received more than 24 bytes, there might be another message
				// In a production system, you'd want to buffer partial messages
				if (BytesRead > 24)
				{
					UE_LOG(LogTemp, Warning, TEXT("SocketCommunication: Received %d bytes, expected 24. Extra data discarded."), BytesRead);
				}
			}
			else
			{
				// Partial receive - in a production system, you'd buffer this
				UE_LOG(LogTemp, Warning, TEXT("SocketCommunication: Received partial message (%d/24 bytes)"), BytesRead);
			}
		}
		else if (!bRead)
		{
			// Connection might have been closed
			ESocketErrors ErrorCode = SocketSubsystem->GetLastErrorCode();
			if (ErrorCode != SE_NO_ERROR && ErrorCode != SE_EWOULDBLOCK)
			{
				UE_LOG(LogTemp, Warning, TEXT("SocketCommunication: Error receiving data. Error code: %d"), (int32)ErrorCode);
				// Close and clean up the client socket
				ClientSocket->Close();
				SocketSubsystem->DestroySocket(ClientSocket);
				ClientSocket = nullptr;
			}
		}
	}
}

void USocketCommunication::ParseReceivedData(const TArray<uint8>& Data)
{
	if (Data.Num() < 24)
	{
		UE_LOG(LogTemp, Warning, TEXT("SocketCommunication: Cannot parse data - insufficient bytes (%d/24)"), Data.Num());
		return;
	}

	// Parse the data in the same format as sent by the client:
	// Velocity.X, Velocity.Y, Velocity.Z, Rotation.Pitch, Rotation.Yaw, Rotation.Roll
	const uint8* DataPtr = Data.GetData();

	// Read velocity components
	float VelX, VelY, VelZ;
	FMemory::Memcpy(&VelX, DataPtr, sizeof(float));
	FMemory::Memcpy(&VelY, DataPtr + sizeof(float), sizeof(float));
	FMemory::Memcpy(&VelZ, DataPtr + (2 * sizeof(float)), sizeof(float));

	// Read rotation components
	float Pitch, Yaw, Roll;
	FMemory::Memcpy(&Pitch, DataPtr + (3 * sizeof(float)), sizeof(float));
	FMemory::Memcpy(&Yaw, DataPtr + (4 * sizeof(float)), sizeof(float));
	FMemory::Memcpy(&Roll, DataPtr + (5 * sizeof(float)), sizeof(float));

	// Update the stored values
	IncomingVelocity = FVector(VelX, VelY, VelZ);
	IncomingRotation = FRotator(Pitch, Yaw, Roll);
}
