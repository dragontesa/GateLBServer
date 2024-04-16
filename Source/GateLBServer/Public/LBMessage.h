// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NetworkMessage.h"

enum GATELBSERVER_API ELBMessageType
{
	ELBMessage_MatchGameServer = 1000,
	ELBMessage_IncreaseGamePlayer,
	ELBMessage_DecreaseGamePlayer,
};

struct GATELBSERVER_API FLBMessageHeader
{
	uint32 MessageId;
	uint32 DataSize;
	FLBMessageHeader():
	    MessageId(0), DataSize(0) {
	}
	
	FLBMessageHeader(uint32 id, uint32 size):
	    MessageId(id), DataSize(size) {

	}

	friend FArchive& operator<<(FArchive& Ar, FLBMessageHeader Header)
	{
		Ar << Header.MessageId;
		Ar << Header.DataSize;
		return Ar;
	}

};

/**
 * 통신의 기본 트랜젝션 정보로 서버-클라이언트 간 송수신 패킷에 대한 신뢰 검증, 모든 패킷에 포함
*/
struct GATELBSERVER_API FLBMessageToken
{
	uint32 TransactionId;
	uint32 GameHostId;
	uint32 GameId;
	uint32 AuthKey;

    FLBMessageToken()
	{
		TransactionId = 0;
		GameHostId = 0;
		GameId = 0;
		AuthKey = 0;
	}

	friend FArchive& operator<<(FArchive& Ar, FLBMessageToken Token)
	{
		Ar << Token.TransactionId;
		Ar << Token.GameHostId;
		Ar << Token.GameId;
		Ar << Token.AuthKey;
		return Ar;
	}
};


/**
 * 매칭 게임서버 정보 메세지
*/
struct GATELBSERVER_API FLBGameServerMatchMessage
{
    uint16  Port;
    uint16  Padding;
    FString ServerIp4;
    FString ServerIp6;

	friend FArchive& operator<<(FArchive& Ar, FLBGameServerMatchMessage Message)
	{
		Ar << Message.Port;
		Ar << Message.Padding;
		Ar << Message.ServerIp4;
		Ar << Message.ServerIp6;

		return Ar;
	}
};


/**
 *   네트워크 전송용 메세지
 */
class GATELBSERVER_API FLBMessage: public FLBMessageHeader
{
public:
    FLBMessage() = default;
	FLBMessage(FLBMessageHeader Header);
	FLBMessage(uint32 NewMessageId, uint32 NewDataSize=0);
	virtual ~FLBMessage();

	FLBMessageHeader MessageHeader() const;
	const TArray<uint8>& MessageData() const;
	int32 DataBody(TArray<uint8>& OutData) const; // Other exclude Token
	TSharedPtr<FBufferArchive> Packet();
	int32 SetData(const TArray<uint8>& InData);
	int32 SetData(const FLBGameServerMatchMessage& Message);
	int32 SetData(int32 NewGameId);
	int32 SetData(const FString& NewData);
	int32 GetServerMatchMessage(FLBGameServerMatchMessage& OutMessage) const;
	int32 GetIncreaseGamePlayerMessage(int32& OutMessage) const;
	int32 GetDecreaseGamePlayerMessage(int32& OutMessage) const;

	FLBMessageToken MessageToken() const;
	void SetMessageToken(FLBMessageToken NewToken);

	private:
	FLBMessageToken ThisToken;
	TArray<uint8> Data;
};
