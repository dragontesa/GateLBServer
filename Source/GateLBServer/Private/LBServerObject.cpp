// Fill out your copyright notice in the Description page of Project Settings.


#include "LBServerObject.h"

DEFINE_LOG_CATEGORY(GateLBServerLog);

FLBServerJob::FLBServerJob(const JobContextType& context)
{
   Server       = nullptr;
   Customer     = context.Customer;
   MessageToken = context.Token;
   MessageType  = context.MessageType;
}

FLBServerJob::~FLBServerJob()
{
   Customer->Finish();
}

void FLBServerJob::Execute()
{
   switch (MessageType)
   {
      case ELBMessageType::ELBMessage_MatchGameServer:
      {
         HandleServerMatchResult(RequestFindServerMatch());
      }
      break;
      case ELBMessageType::ELBMessage_IncreaseGamePlayer:
      {
         RequestIncreaseGamePlayer();
      }
      break;
      case ELBMessageType::ELBMessage_DecreaseGamePlayer:
      {
         RequestDecreaseGamePlayer();
      }
      break;
      default:
      break;
   }
}

TFuture<FLBServerMatchResult> 
FLBServerJob::RequestFindServerMatch()
{
   TSharedPtr<TPromise<FLBServerMatchResult>> MatchPromise = MakeShared<TPromise<FLBServerMatchResult>>();
            AsyncTask(ENamedThreads::GameThread, [this, MatchPromise]() {
               Server->HandleFindServerMatch(MessageToken.GameId, MatchPromise);
            });
   
    return MatchPromise->GetFuture();
}

void FLBServerJob::RequestIncreaseGamePlayer()
{
   Server->HandleIncreaseGamePlayer(MessageToken.GameId);
}

void FLBServerJob::RequestDecreaseGamePlayer()
{
   Server->HandleDecreaseGamePlayer(MessageToken.GameId);
}


void FLBServerJob::HandleServerMatchResult(TFuture<FLBServerMatchResult> MatchResult)
{
    MatchResult.Next([this](FLBServerMatchResult result)
    {
        FLBMessage Message((uint32)ELBMessageType::ELBMessage_MatchGameServer);
        FLBGameServerMatchMessage MatchData;
        result >> MatchData;
        Message.SetMessageToken(MessageToken);
        Message.SetData(MatchData);
        TSharedPtr<FBufferArchive> Packet = Message.Packet();
        Customer->SendAsync(Packet);
    });
}


ULBServerObject::ULBServerObject()
{
   AddToRoot();
   // Initialize Server Socket
   LBServerListenSock.Initialize(this);
   LBServerListenSock.ServerListen();
   // LBServerListenSock.FindServerMatchDelegate.BindUObject(this,ULBServerObject::HandleFindServerMatch);
}

ULBServerObject::~ULBServerObject()
{

}


void ULBServerObject::RequestServerJob(TSharedPtr<FLBServerJob> Job)
{
   Job->Server = this;
   JobQueue.Enqueue(Job);
   while (!JobQueue.IsEmpty())
   {
         TSharedPtr<FLBServerJob> J;
         JobQueue.Dequeue(J);
         AsyncTask(ENamedThreads::AnyThread, [this,J]()
         {
            J->Execute();
         });
   }
}

// This'll be called from GameThread 
void ULBServerObject::AddGameServerMatch(int32 ServerId, const FLBServerMatchInfo& ServerMatch)
{
   MatchServerMap.Add(ServerId, ServerMatch);
}

void ULBServerObject::RemoveGameServerMatch(int32 ServerId)
{
   MatchServerMap.Remove(ServerId);
}

void ULBServerObject::HandleFindServerMatch(int32 GameId, TSharedPtr<TPromise<FLBServerMatchResult>> MatchPromise)
{
   auto Promise = MatchPromise;
   // TODO Find lazy GameServer from List in Database
   auto top = MatchServerMap.begin().Value();
   int32 id = MatchServerMap.begin().Key();

   for (auto&& [k,v]: MatchServerMap)
   {
      if (v.GamePlayerNumbers == 0)
      {
         id = k;
         top = v;
         top.GameId = GameId;
         top.GamePlayerNumbers = 1;
         break;
      }
   }

   FLBServerMatchResult match;
   match.GameId = GameId; // this should be filled
   match.Port = top.Port;
   match.ServerIp4 = top.ServerIp4;
   match.ServerIp6 = top.ServerIp6;
   Promise->SetValue(match);

   // Notify to UI
   AsyncTask(ENamedThreads::GameThread, [this,id,GameId]()
   {
      UpdateGameServerMatchEvent.Broadcast(id, GameId);
   });
}


void ULBServerObject::HandleIncreaseGamePlayer(int32 GameId)
{
   int32 id = MatchServerMap.begin().Key();
   for (auto&& [k,v]: MatchServerMap)
   {
      if (v.GameId == GameId)
      {
         id = k;
         v.GamePlayerNumbers++;
         break;
      }
   }
   
   // Notify to UI
   AsyncTask(ENamedThreads::GameThread, [this,id,GameId]()
   {
      IncreaseGamePlayerEvent.Broadcast(id, GameId);
   });
}

void ULBServerObject::HandleDecreaseGamePlayer(int32 GameId)
{
   int32 id = MatchServerMap.begin().Key();
   for (auto&& [k,v]: MatchServerMap)
   {
      if (v.GameId == GameId)
      {
         id = k;
         v.GamePlayerNumbers--;
         break;
      }
   }

   // Notify to UI
   AsyncTask(ENamedThreads::GameThread, [this,id,GameId]()
   {
      DecreaseGamePlayerEvent.Broadcast(id, GameId);
   });
}

