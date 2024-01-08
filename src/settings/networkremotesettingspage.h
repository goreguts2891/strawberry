#ifndef NETWORKREMOTESETTINGSPAGE_H
#define NETWORKREMOTESETTINGSPAGE_H

#include <QWidget>
#include <QObject>
#include <QString>
#include <QSpinBox>

#include "settingspage.h"

class SettingsDialog;
class Ui_NetworkRemoteSettingsPage;

class NetworkRemoteSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    explicit NetworkRemoteSettingsPage(SettingsDialog *dialog, QWidget *parent = nullptr);
    ~NetworkRemoteSettingsPage() override;

    static const char *kSettingsGroup;

    void Load() override;
    void Save() override;



private:
  Ui_NetworkRemoteSettingsPage *ui_;
  QSettings s;
  //void EnableRemote();
  //void LocalConnectOnly();
  void DisplayIP();

private slots:
  void RemoteButtonClicked();
};

#endif // NETWORKREMOTESETTINGSPAGE_H