/*
   Copyright (C) 2014-2017 Alexandr Akulich <akulichalexander@gmail.com>

   This file is a part of TelegramQt library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

 */

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QMap>

#include "TelegramNamespace.hpp"
#include "ConnectionApi.hpp"

namespace Ui {
class MainWindow;
}

namespace Telegram {

namespace Client {

class Client;

} // Client namespace

} // Telegram namespace

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum AppState {
        AppStateNone,
        AppStateDisconnected,
        AppStateConnected,
        AppStateAuthRequired,
        AppStateCodeRequested,
        AppStateCodeSent,
        AppStateCodeProvided,
        AppStatePasswordRequired,
        AppStatePasswordProvided,
        AppStateSignedIn,
        AppStateReady,
        AppStateLoggedOut,
    };
    Q_ENUM(AppState)

protected slots:
    void onConnectionStateChanged(Telegram::Client::ConnectionApi::Status status);
    void onLoggedOut(bool result);
    void onPhoneStatusReceived(const QString &phone, bool registered);
    void onPhoneCodeRequested();
    void onPasswordInfoReceived(quint64 requestId);
    void onUnauthorizedErrorReceived(TelegramNamespace::UnauthorizedError errorCode, const QString &errorMessage);
    void onAuthSignErrorReceived(TelegramNamespace::AuthSignError errorCode, const QString &errorMessage);

    void on_mainSplitter_splitterMoved(int pos, int index);
    void on_connectionStepButton_clicked();
    void on_secondConnectButton_clicked();

    void getConnectionSecretInfo();

    void on_phoneNumber_returnPressed();
    void on_confirmationCode_returnPressed();
    void on_password_returnPressed();
    void on_setStatusOnline_clicked();
    void on_setStatusOffline_clicked();
    void on_logoutButton_clicked();
    void on_disconnectButton_clicked();

    void on_secretSaveAs_clicked();

protected:
    void closeEvent(QCloseEvent *event);

    void initStartConnection();
    void initRequestAuthCode();
    void initTryAuthCode();
    void initTryPassword();
    void initLogout();

private slots:
    void on_restoreSession_clicked();

    void loadSecretFromBrowsedFile();

    void setUiProxyEnabled(bool enabled);
    void setWorkLikeAClient(bool enabled);
    void updateClientUi();

private:
    void setAppState(AppState newState);
    void setRegistered(bool newRegistered);

    Ui::MainWindow *ui = nullptr;

    Telegram::Client::Client *m_backend = nullptr;

//    Telegram::PasswordInfo *m_passwordInfo;
    QMap<quint32,quint64> m_contactLastMessageList;
    QMap<quint32, Telegram::Peer> m_uploadingRequests;

    bool m_registered = false;
    bool m_workLikeAClient;
    bool m_phoneNumberSubmitted = false;

    AppState m_appState;

};

#endif // MAINWINDOW_HPP
