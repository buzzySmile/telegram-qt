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

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "CAppInformation.hpp"
#include "DataStorage.hpp"
#include "Debug.hpp"
#include "Client.hpp"

#include "ContactsApi.hpp"
#include "ConnectionApi.hpp"
#include "MessagingApi.hpp"

//#include "CContactModel.hpp"
//#include "CContactsFilterModel.hpp"
//#include "CDialogModel.hpp"
//#include "CMessageModel.hpp"
//#include "CChatInfoModel.hpp"
//#include "CFileManager.hpp"

#include <QCompleter>
#include <QToolTip>
#include <QStringListModel>

#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>

#include <QDebug>

#include <QFile>
#include <QFileDialog>
#include <QMetaEnum>

static const int c_peerPictureColumnWidth = 70;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_backend(new Telegram::Client::Client(this)),
    // m_passwordInfo(nullptr),
    m_workLikeAClient(true),
    m_appState(AppStateNone)
{
    ui->setupUi(this);
    ui->workLikeClient->setChecked(m_workLikeAClient);
    connect(ui->workLikeClient, &QAbstractButton::toggled, this, &MainWindow::setWorkLikeAClient);

    // Telepathy Morse app info
    CAppInformation *appInfo = new CAppInformation(this);
    appInfo->setAppId(14617);
    appInfo->setAppHash(QLatin1String("e17ac360fd072f83d5d08db45ce9a121"));
    appInfo->setAppVersion(QLatin1String("0.1"));
    appInfo->setDeviceInfo(QLatin1String("pc"));
    appInfo->setOsInfo(QLatin1String("GNU/Linux"));
    appInfo->setLanguageCode(QLatin1String("en"));

    m_backend->setAppInformation(appInfo);

    connect(m_backend, SIGNAL(connectionStateChanged(TelegramNamespace::ConnectionState)),
            SLOT(onConnectionStateChanged(TelegramNamespace::ConnectionState)));
    connect(m_backend, SIGNAL(phoneStatusReceived(QString,bool)),
            SLOT(onPhoneStatusReceived(QString,bool)));
    connect(m_backend, SIGNAL(phoneCodeRequired()),
            SLOT(onPhoneCodeRequested()));
    connect(m_backend, SIGNAL(passwordInfoReceived(quint64)),
            SLOT(onPasswordInfoReceived(quint64)));
    connect(m_backend, SIGNAL(authorizationErrorReceived(TelegramNamespace::UnauthorizedError,QString)),
            SLOT(onUnauthorizedErrorReceived(TelegramNamespace::UnauthorizedError,QString)));
    connect(m_backend, SIGNAL(authSignErrorReceived(TelegramNamespace::AuthSignError,QString)),
            SLOT(onAuthSignErrorReceived(TelegramNamespace::AuthSignError,QString)));

    ui->mainSplitter->setStretchFactor(1, 1);
    ui->mainSplitter->setSizes(QList<int>() << 10 << 100);

    QFile helpFile(QLatin1String(":/USAGE"));
    helpFile.open(QIODevice::ReadOnly);
    ui->helpView->setPlainText(helpFile.readAll());

    setUiProxyEnabled(false);
    setAppState(AppStateDisconnected);

    connect(ui->settingsProxyEnable, SIGNAL(toggled(bool)), this, SLOT(setUiProxyEnabled(bool)));

    updateClientUi();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onConnectionStateChanged(Telegram::Client::ConnectionApi::Status status)
{
    switch (status) {
    case Telegram::Client::ConnectionApi::StatusConnected:
        setAppState(AppStateConnected);
        break;
    case Telegram::Client::ConnectionApi::StatusWaitForAuthentication:
        setAppState(AppStateAuthRequired);
        break;
//    case Telegram::Client::ConnectionApi::ConnectionStateAuthenticated:
//        setAppState(AppStateSignedIn);
//        break;
    case Telegram::Client::ConnectionApi::StatusReady:
        setAppState(AppStateReady);
        break;
    case Telegram::Client::ConnectionApi::StatusDisconnected:
        setAppState(AppStateDisconnected);
        break;
    default:
        break;
    }
}

void MainWindow::onLoggedOut(bool result)
{
    qDebug() << Q_FUNC_INFO << result;
    setAppState(AppStateLoggedOut);
}

void MainWindow::onPhoneStatusReceived(const QString &phone, bool registered)
{
    if (phone == ui->phoneNumber->text()) {
        QString registeredText = registered ? tr("Registered") : tr("Not registered");
        ui->phoneStatus->setText(registeredText);

        setRegistered(registered);
    } else {
        qDebug() << "Warning: Received status for different phone number" << phone << registered;
    }
}

void MainWindow::onPhoneCodeRequested()
{
    setAppState(AppStateCodeSent);

//    m_backend->requestPhoneStatus(ui->phoneNumber->text());
}

void MainWindow::onPasswordInfoReceived(quint64 requestId)
{
//    qDebug() << Q_FUNC_INFO << requestId;
//    if (!m_passwordInfo) {
//        m_passwordInfo = new Telegram::PasswordInfo();
//    }

//    m_backend->getPasswordInfo(m_passwordInfo, requestId);
}

void MainWindow::onUnauthorizedErrorReceived(TelegramNamespace::UnauthorizedError errorCode, const QString &errorMessage)
{
//    QToolTip::showText(ui->confirmationCode->mapToGlobal(QPoint(0, 0)), errorMessage);
//    qDebug() << errorCode << errorMessage;

//    if (errorCode == TelegramNamespace::UnauthorizedSessionPasswordNeeded) {
//        setAppState(AppStatePasswordRequired);
//        m_backend->getPassword();
//    }
}

void MainWindow::onAuthSignErrorReceived(TelegramNamespace::AuthSignError errorCode, const QString &errorMessage)
{
    switch (errorCode) {
    case TelegramNamespace::AuthSignErrorPhoneNumberIsInvalid:
        if (m_appState == AppStateCodeRequested) {
            QToolTip::showText(ui->phoneNumber->mapToGlobal(QPoint(0, 0)), tr("Phone number is not valid"));
            setAppState(AppStateDisconnected);
        }
        break;
    case TelegramNamespace::AuthSignErrorPhoneCodeIsExpired:
        QToolTip::showText(ui->confirmationCode->mapToGlobal(QPoint(0, 0)), tr("Phone code is expired"));
        break;
    case TelegramNamespace::AuthSignErrorPhoneCodeIsInvalid:
        QToolTip::showText(ui->confirmationCode->mapToGlobal(QPoint(0, 0)), tr("Phone code is invalid"));
        break;
    default:
        qDebug() << "Unknown auth sign error:" << errorMessage;
        return;
    }

    ui->confirmationCode->setFocus();
}

void MainWindow::on_mainSplitter_splitterMoved(int pos, int index)
{
    Q_UNUSED(index)
    ui->actionShowSettings->setChecked(pos > 0);
}

void MainWindow::on_connectionStepButton_clicked()
{
    switch (m_appState) {
    case AppStateNone:
        break;
    case AppStateDisconnected:
        initStartConnection();
        break;
    case AppStateConnected: // Not used by TelegramQt ATM
        break;
    case AppStateAuthRequired:
        initRequestAuthCode();
        break;
    case AppStateCodeRequested:
    case AppStateCodeSent:
        initTryAuthCode();
    case AppStateCodeProvided:
        break;
    case AppStatePasswordRequired:
        initTryPassword();
        break;
    case AppStatePasswordProvided:
    case AppStateSignedIn:
    case AppStateReady:
    case AppStateLoggedOut:
        break;
    }
}

void MainWindow::initStartConnection()
{
    TelegramNamespace::MessageFlags flags = TelegramNamespace::MessageFlagNone;
    if (ui->settingsReceivingFilterReadMessages->isChecked()) {
        flags |= TelegramNamespace::MessageFlagRead;
    }
    if (ui->settingsReceivingFilterOutMessages->isChecked()) {
        flags |= TelegramNamespace::MessageFlagOut;
    }
    if (ui->settingsReceivingFilterForwardedMessages->isChecked()) {
        flags |= TelegramNamespace::MessageFlagForwarded;
    }
//    m_backend->setMessageReceivingFilter(flags);
//    if (ui->settingsProxyEnable->isChecked()) {
//        QNetworkProxy proxySettings;
//        proxySettings.setType(QNetworkProxy::Socks5Proxy);
//        proxySettings.setUser(ui->settingsProxyUser->text());
//        proxySettings.setPassword(ui->settingsProxyPassword->text());
//        proxySettings.setHostName(ui->settingsProxyHost->text());
//        proxySettings.setPort(ui->settingsProxyPort->value());
//        m_backend->setProxy(proxySettings);
//    }
//    m_backend->setUpdatesEnabled(ui->settingsUpdatesEnabled->isChecked());

//    if (ui->testingDcRadio->isChecked()) {
//        m_backend->setServerConfiguration({Telegram::DcOption(QLatin1String("149.154.175.10"), 443)});
//    }
//    const QByteArray secretInfo = QByteArray::fromHex(ui->secretInfo->toPlainText().toLatin1());
//    if (!secretInfo.isEmpty()) {
//        m_backend->setSecretInfo(secretInfo);
//    }
    // NOTE: use connection settings from here
////    m_core->setServerConfiguration({Telegram::DcOption(QStringLiteral("127.0.0.1"), 11443)});
////    const Telegram::RsaKey key = Telegram::RsaKey::fromFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + QStringLiteral("/TelegramServer/public_key.pem"));
//    const Telegram::RsaKey key = m_backend->defaultServerPublicRsaKey();
//    if (!key.isValid()) {
//        qCritical() << "Unable to read RSA key";
//    }
//    m_backend->setServerPublicRsaKey(key);
//    if (!m_backend->connectToServer()) {
//        qWarning() << Q_FUNC_INFO << "Unable to connect";
//    }
}

void MainWindow::on_secondConnectButton_clicked()
{
    ui->tabWidget->setCurrentWidget(ui->tabMain);
    on_connectionStepButton_clicked();
}

void MainWindow::initRequestAuthCode()
{
//    if (m_backend->requestAuthCode(ui->phoneNumber->text())) {
//        m_phoneNumberSubmitted = false;
//        setAppState(AppStateCodeRequested);
//    } else {
//        qWarning() << Q_FUNC_INFO << "Unable to request an auth code";
//    }
}

void MainWindow::initTryAuthCode()
{
    if (m_registered) {
        // m_backend->signIn(ui->phoneNumber->text(), ui->confirmationCode->text());
    } else {
        // m_backend->signUp(ui->phoneNumber->text(), ui->confirmationCode->text(), ui->firstName->text(), ui->lastName->text());
    }
    setAppState(AppStateCodeProvided);
}

void MainWindow::initTryPassword()
{
    if (m_appState == AppStatePasswordRequired) {
        // m_backend->tryPassword(m_passwordInfo->currentSalt(), ui->password->text());
        setAppState(AppStatePasswordProvided);
    } else {
        qWarning() << Q_FUNC_INFO << "Password is not required";
    }
}

void MainWindow::initLogout()
{
    if (m_appState >= AppStateSignedIn) {
        // m_backend->logOut();
    } else {
        qWarning() << Q_FUNC_INFO << "Not signed in to logout";
    }
}

void MainWindow::getConnectionSecretInfo()
{
    // ui->secretInfo->setPlainText(m_backend->connectionSecretInfo().toHex());
}

void MainWindow::setRegistered(bool newRegistered)
{
    m_registered = newRegistered;

    ui->firstName->setDisabled(m_registered);
    ui->firstNameLabel->setDisabled(m_registered);
    ui->lastName->setDisabled(m_registered);
    ui->lastNameLabel->setDisabled(m_registered);

//    if (m_registered) {
//        ui->signButton->setText(tr("Sign in"));
//    } else {
//        ui->signButton->setText(tr("Sign up"));
//    }
}

void MainWindow::setAppState(MainWindow::AppState newState)
{
    const auto formatName = [](const AppState state) {
        static const QMetaEnum appStateEnum = QMetaEnum::fromType<AppState>();
        return appStateEnum.key(state);
    };
    qDebug() << "Change app state from" << formatName(m_appState) << "to" << formatName(newState);
    m_appState = newState;

    ui->confirmationCode->setEnabled(m_appState == AppStateCodeSent);
    ui->password->setEnabled(m_appState == AppStatePasswordRequired);

    ui->setStatusOnline->setVisible((m_appState >= AppStateSignedIn) && !m_workLikeAClient);
    ui->setStatusOffline->setVisible((m_appState >= AppStateSignedIn) && !m_workLikeAClient);

    ui->phoneNumber->setEnabled(m_appState < AppStateCodeSent);
    ui->connectionStepButton->setVisible(m_appState < AppStateSignedIn);
    ui->restoreSession->setVisible(m_appState == AppStateDisconnected);

    ui->logoutButton->setVisible((m_appState == AppStateSignedIn) || (m_appState == AppStateReady));
    ui->disconnectButton->setVisible(m_appState >= AppStateConnected);

    switch (m_appState) {
    case AppStateNone:
        // Fall throught
    case AppStateDisconnected:
        ui->connectionState->setText(tr("Disconnected"));
        ui->connectionStepButton->setText(tr("Connect"));
        ui->connectionStepButton->setEnabled(true);
        ui->phoneNumber->setEnabled(true);
        ui->phoneNumber->setFocus();
        break;
    case AppStateConnected: // Not used by TelegramQT ATM
    case AppStateAuthRequired:
        ui->connectionState->setText(tr("Connected..."));
        ui->connectionStepButton->setText(tr("Send the phone number"));
        ui->connectionStepButton->setEnabled(true);
        if (m_phoneNumberSubmitted) {
            initRequestAuthCode();
        } else {
            ui->phoneNumber->setFocus();
        }
        break;
    case AppStateCodeRequested:
        ui->connectionState->setText(tr("An auth code is requested..."));
        break;
    case AppStateCodeSent:
        ui->connectionState->setText(tr("The auth code is sent..."));
        ui->connectionStepButton->setText(tr("Try the auth code"));
        ui->confirmationCode->setFocus();
        break;
    case AppStateCodeProvided:
        ui->connectionState->setText(tr("Trying the auth code..."));
        ui->connectionStepButton->setEnabled(false);
        ui->confirmationCode->setFocus();
        break;
    case AppStatePasswordRequired:
        ui->connectionState->setText(tr("Password required"));
        ui->connectionStepButton->setText(tr("Try the password"));
        ui->connectionStepButton->setEnabled(true);
        ui->password->setFocus();
        break;
    case AppStatePasswordProvided:
        ui->connectionStepButton->setEnabled(false);
        ui->connectionState->setText(tr("Trying the password..."));
        ui->connectionStepButton->setEnabled(false);
        break;
    case AppStateSignedIn:
        ui->connectionState->setText(tr("Signed in..."));
        if (m_workLikeAClient) {
            // TODO: m_backend->setOnlineStatus(true);
        }
        break;
    case AppStateReady:
        ui->connectionState->setText(tr("Ready"));
        // TODO: // ui->phoneNumber->setText(m_backend->selfPhone());
        // updateContactList();
    {
        Telegram::UserInfo selfInfo;
        m_backend->dataStorage()->getUserInfo(&selfInfo, m_backend->dataStorage()->selfUserId());
        ui->firstName->setText(selfInfo.firstName());
        ui->lastName->setText(selfInfo.lastName());
    }
        break;
    case AppStateLoggedOut:
        ui->connectionState->setText(tr("Logged out"));
        break;
    default:
        break;
    }
}

void MainWindow::setUiProxyEnabled(bool enabled)
{
    ui->settingsProxyEnable->setChecked(enabled);
    ui->settingsProxyUser->setEnabled(enabled);
    ui->settingsProxyPassword->setEnabled(enabled);
    ui->settingsProxyHost->setEnabled(enabled);
    ui->settingsProxyPort->setEnabled(enabled);
}

void MainWindow::setWorkLikeAClient(bool enabled)
{
    if (m_workLikeAClient == enabled) {
        return;
    }
    m_workLikeAClient = enabled;
    updateClientUi();
}

void MainWindow::updateClientUi()
{
    ui->setStatusOffline->setVisible(!m_workLikeAClient);
    ui->setStatusOnline->setVisible(!m_workLikeAClient);
}

void MainWindow::on_phoneNumber_returnPressed()
{
    m_phoneNumberSubmitted = !ui->phoneNumber->text().isEmpty();
    if (m_appState == AppStateDisconnected) {
        initStartConnection();
    } else if (m_appState == AppStateAuthRequired) {
        initRequestAuthCode();
    }
}

void MainWindow::on_confirmationCode_returnPressed()
{
    if (m_appState != AppStateCodeSent) {
        return;
    }
    initTryAuthCode();
}

void MainWindow::on_password_returnPressed()
{
    if (m_appState != AppStatePasswordRequired) {
        return;
    }
    initTryPassword();
}

void MainWindow::on_setStatusOnline_clicked()
{
    // m_backend->setOnlineStatus(/* online */ true);
}

void MainWindow::on_setStatusOffline_clicked()
{
    // m_backend->setOnlineStatus(/* online */ false);
}

void MainWindow::on_logoutButton_clicked()
{
    initLogout();
}

void MainWindow::on_disconnectButton_clicked()
{
    m_backend->connectionApi()->disconnectFromServer();
    // m_backend->resetConnectionData();
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)

//    if (m_workLikeAClient) {
//        m_backend->setOnlineStatus(false);
//    }
}


void MainWindow::on_secretSaveAs_clicked()
{
    static const auto secretFileNameExtension = QStringLiteral(".tgsecret");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save secret info..."), QString(), tr("Telegram secret files (*%1)").arg(secretFileNameExtension));
    if (fileName.isEmpty()) {
        return;
    }

    if (!fileName.endsWith(secretFileNameExtension)) {
        fileName += secretFileNameExtension;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    if (ui->secretInfo->toPlainText().isEmpty()) {
        getConnectionSecretInfo();
    }

    file.write(ui->secretInfo->toPlainText().toLatin1());
}

void MainWindow::on_restoreSession_clicked()
{
    loadSecretFromBrowsedFile();

    if (ui->secretInfo->toPlainText().isEmpty()) {
        return;
    }

    on_connectionStepButton_clicked();
}

void MainWindow::loadSecretFromBrowsedFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Load secret info..."), QString(), tr("Telegram secret files (*.tgsecret);;All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    ui->secretInfo->setPlainText(file.readAll());
}
