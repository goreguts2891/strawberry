
#include <QSettings>
#include <QThread>

#include "networkremote/networkremote.h"
#include "core/logging.h"


class TcpServer;

const char *NetworkRemote::kSettingsGroup = "Remote";

NetworkRemote::NetworkRemote(Application* app, QObject *parent)
    : QObject(parent),
      app_(app),
      original_thread_(nullptr)
{
  setObjectName("Network Remote");
  original_thread_ = thread();
}

NetworkRemote::~NetworkRemote()
{
  stopTcpServer();
}

void NetworkRemote::Init()
{
  QSettings s;
  s.beginGroup(NetworkRemote::kSettingsGroup);
  use_remote_ = s.value("useRemote").toBool();
  local_only_ = s.value("localOnly").toBool();
  remote_port_ = s.value("remotePort").toInt();
  ipAddr_.setAddress(s.value("ipAddress").toString());
  s.endGroup();

  if (use_remote_){
    startTcpServer();
  }
  else {
    stopTcpServer();
  }
}

void NetworkRemote::startTcpServer()
{
  server_->StartServer(ipAddr_,remote_port_);
  qLog(Debug) << "TcpServer started on IP " << ipAddr_<< " and port" << remote_port_;
}

void NetworkRemote::stopTcpServer()
{
  if (server_->ServerUp()){
    qLog(Debug) << "TcpServer stopped ";
    server_->StopServer();
  }
}

