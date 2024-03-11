#include "client.h"

Client::Client(Application *app, QObject *parent)
    : QObject{parent},
      app_(app),
      incomingMsg_(new IncomingMsg(app)),
      outgoingMsg_(new OutgoingMsg(app)),
      player_(app_->player())
{
  QObject::connect(&*app_->player()->engine(), &EngineBase::StateChanged, this, &Client::EngineChanged);
}

Client::~Client()
{
  incomingMsg_->deleteLater();
  outgoingMsg_->deleteLater();
}

void Client::Init(QTcpSocket *socket)
{
  socket_ = socket;
  QObject::connect(incomingMsg_,&IncomingMsg::InMsgParsed,this, &Client::ProcessIncoming);

  incomingMsg_->Init(socket_);
  outgoingMsg_->Init(socket_, player_);
}

QTcpSocket* Client::GetSocket()
{
  return socket_;
}

void Client::ProcessIncoming()
{
  msgType_ = incomingMsg_->GetMsgType();
  switch (msgType_)
  {
    case nw::remote::MSG_TYPE_REQUEST_SONG_INFO:
      outgoingMsg_->SendCurrentTrackInfo();
      break;
    case nw::remote::MSG_TYPE_REQUEST_PLAY:
      break;
    case nw::remote::MSG_TYPE_REQUEST_NEXT:
      break;
    case nw::remote::MSG_TYPE_REQUEST_PREVIOUS:
      break;
    case nw::remote::MSG_TYPE_REQUEST_PAUSE:
      break;
    case nw::remote::MSG_TYPE_REQUEST_STOP:
      break;
    case nw::remote::MSG_TYPE_REQUEST_FINISH:
      break;
    case nw::remote::MSG_TYPE_DISCONNECT:
      break;
    default:
        qInfo("Unknown mwessage type");
      break;
}
}

void Client::Respond()
{
  //outgoingMsg_->ProcessMsg(socket_, incomingMsg_->GetMsgType());
}

void Client::EngineChanged()
{
  qInfo("Engine has changed");
}

