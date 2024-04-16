// Fill out your copyright notice in the Description page of Project Settings.


#include "LBMessage.h"
#include "Serialization/ArrayWriter.h"

static TSharedPtr<FBufferArchive> NewMessagePacket(uint32 NewMessageId, const uint8* InData, uint32 NewDataSize);
static TSharedPtr<FBufferArchive> NewMessagePacket(uint32 NewMessageId, const FString& Text);
static TSharedPtr<FBufferArchive> NewMessagePacket(uint32 NewMessageId, const TArray<uint8>& InData);
static TSharedPtr<FBufferArchive> NewMessagePacket(const FLBGameServerMatchMessage& ServerMatchMessage);

TSharedPtr<FBufferArchive> NewMessagePacket(uint32 NewMessageId, const uint8* InData, uint32 NewDataSize)
{
    FLBMessageHeader header(NewMessageId, NewDataSize);
    TSharedPtr<FBufferArchive> packet = MakeShareable(new FBufferArchive());
	(*packet) << header;
    packet->Append(InData, NewDataSize);

    return packet;
}

TSharedPtr<FBufferArchive> NewMessagePacket(uint32 NewMessageId, const FString& Text)
{
    // SCOPE_CYCLE_COUNTER(STAT_Send);

    FTCHARToUTF8 Convert(*Text);
	FArrayWriter WriterArray;

	WriterArray.Serialize((UTF8CHAR*)Convert.Get(), Convert.Length());
	TSharedPtr<FBufferArchive> Packet = NewMessagePacket(NewMessageId, WriterArray.GetData(), WriterArray.Num());
	return Packet;
}

TSharedPtr<FBufferArchive> NewMessagePacket(uint32 NewMessageId, const TArray<uint8>& InData)
{
    return NewMessagePacket(NewMessageId, InData.GetData(), sizeof(uint8)*InData.Num());
}

TSharedPtr<FBufferArchive> NewMessagePacket(const FLBGameServerMatchMessage& ServerMatchMessage)
{
    FBufferArchive DataBuf;
    DataBuf << ServerMatchMessage;
    uint32 DataSize = DataBuf.Num();
    return NewMessagePacket(uint32(ELBMessageType::ELBMessage_MatchGameServer), DataBuf.GetData(), DataSize);
}

TSharedPtr<FBufferArchive> NewMessagePacket(const FString& DecreaseGamePlayerMessage)
{
    FTCHARToUTF8 Convert(*DecreaseGamePlayerMessage);
	FArrayWriter WriterArray;

	WriterArray.Serialize((UTF8CHAR*)Convert.Get(), Convert.Length());

	return NewMessagePacket(uint32(ELBMessageType::ELBMessage_DecreaseGamePlayer), WriterArray.GetData(), WriterArray.Num());
}

TSharedPtr<FBufferArchive> NewMessagePacket(int64 DecreaseGamePlayerMessage)
{
	FBufferArchive DataBuf;
    DataBuf << DecreaseGamePlayerMessage;

	return NewMessagePacket(uint32(ELBMessageType::ELBMessage_DecreaseGamePlayer), DataBuf.GetData(), DataBuf.Num());
}

FLBMessage::FLBMessage(FLBMessageHeader Header)
  : FLBMessageHeader(Header)
{
}

FLBMessage::FLBMessage(uint32 NewMessageId, uint32 NewDataSize)
  : FLBMessageHeader(NewMessageId, NewDataSize)
{
}

FLBMessage::~FLBMessage()
{
}

FLBMessageHeader FLBMessage::MessageHeader() const
{
    return FLBMessageHeader(MessageId, DataSize);
}

const TArray<uint8>& FLBMessage::MessageData() const
{
    return Data;
}

int32 FLBMessage::DataBody(TArray<uint8>& OutData) const
{
    if (Data.Num() <= sizeof(FLBMessageToken))
    {
        return 0;
    }

    FLBMessageToken token;
    FMemoryReader reader(Data);
    reader << token;
    reader << OutData;

    return OutData.Num();
}

int32 FLBMessage::SetData(const TArray<uint8>& InData)
{
    Data.Empty();
    Data.SetNumZeroed(InData.Num());
    Data = InData;
    DataSize = InData.Num();
    return DataSize;
}

int32 FLBMessage::SetData(const FLBGameServerMatchMessage& Message)
{
    FBufferArchive Buffer;
    Buffer << Message;
    Data.Empty();
    Data = TArray<uint8>(Buffer);
    DataSize = Data.Num();
    return DataSize;
}

int32 FLBMessage::SetData(int32 NewGameId)
{
    FBufferArchive Buffer;
    Buffer << NewGameId;
    Data.Empty();
    Data = TArray<uint8>(Buffer);
    DataSize = Data.Num();
    return DataSize;
}

int32 FLBMessage::SetData(const FString& NewData)
{
    FTCHARToUTF8 Convert(*NewData);
	FArrayWriter WriterArray;

	WriterArray.Serialize((UTF8CHAR*)Convert.Get(), Convert.Length());
    Data.Empty();
    Data = TArray<uint8>(WriterArray);
    DataSize = WriterArray.Num();
    return DataSize;
}

void FLBMessage::SetMessageToken(FLBMessageToken NewToken)
{
    ThisToken = NewToken;
}


TSharedPtr<FBufferArchive> FLBMessage::Packet()
{
    // Pack MessageToken in Data
    FBufferArchive DataBuff;
    DataBuff << ThisToken;

    if (Data.Num() > 0)
       DataBuff.Append(Data.GetData(), Data.Num());
    
    return NewMessagePacket(MessageId, DataBuff.GetData(), DataBuff.Num());
}

FLBMessageToken FLBMessage::MessageToken() const
{
    if (Data.IsEmpty() || Data.Num() < sizeof(FLBMessageToken))
    {
        UE_LOG(LogTemp, Log, TEXT("LBMessage Invalid Token."));
        return FLBMessageToken();
    }

    FLBMessageToken token;
    FMemoryReader reader(Data);
    reader << token;

    return token;
}


int32 FLBMessage::GetServerMatchMessage(FLBGameServerMatchMessage& OutMessage) const
{
    if (Data.IsEmpty() || Data.Num() < sizeof(FLBMessageToken))
    {
        UE_LOG(LogTemp, Log, TEXT("LBMessage Invalid Message Data."));
        return 0;
    }

    FLBMessageToken token;
    FLBGameServerMatchMessage msg;
    FMemoryReader reader(Data);
    reader << token;
    reader << msg;

    OutMessage = msg;

    return 1;
}

int32 FLBMessage::GetIncreaseGamePlayerMessage(int32& OutMessage) const
{
    if (Data.IsEmpty() || Data.Num() < sizeof(FLBMessageToken)+sizeof(int64))
    {
        UE_LOG(LogTemp, Log, TEXT("LBMessage Invalid Message Data."));
        return 0;
    }

    FLBMessageToken token;
    FMemoryReader reader(Data);
    reader << token;

    OutMessage = token.GameId;

    return 1;
}

int32 FLBMessage::GetDecreaseGamePlayerMessage(int32& OutMessage) const
{
    if (Data.IsEmpty() || Data.Num() < sizeof(FLBMessageToken)+sizeof(int64))
    {
        UE_LOG(LogTemp, Log, TEXT("LBMessage Invalid Message Data."));
        return 0;
    }

    FLBMessageToken token;
    FMemoryReader reader(Data);
    reader << token;

    OutMessage = token.GameId;

    return 1;
}

