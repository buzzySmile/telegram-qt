#include "TelegramServerUser.hpp"

#include "ApiUtils.hpp"
#include "ServerApi.hpp"
#include "ServerRpcLayer.hpp"
#include "ServerUtils.hpp"
#include "Session.hpp"
#include "RandomGenerator.hpp"
#include "Utils.hpp"

#include <QCryptographicHash>
#include <QLoggingCategory>

namespace Telegram {

namespace Server {

quint32 PostBox::addMessage(const TLMessage &message)
{
    ++m_lastMessageId;
    ++m_pts;

    m_messages.insert(m_lastMessageId, message);
    TLMessage *addedMessage = &m_messages[m_lastMessageId];
    addedMessage->id = m_lastMessageId;
    return m_lastMessageId;
}

const TLMessage *PostBox::getMessage(quint32 messageId) const
{
    PostBox *mutableBox = const_cast<PostBox *>(this);
    return mutableBox->getMutableMessage(messageId);
}

TLMessage *PostBox::getMutableMessage(quint32 messageId)
{
    if (m_messages.contains(messageId)) {
        return &m_messages[messageId];
    }
    return nullptr;
}

TLPeer MessageRecipient::toTLPeer() const
{
    const Peer p = toPeer();
    if (Q_UNLIKELY(!p.isValid())) {
        qCritical() << Q_FUNC_INFO << "Invalid peer" << this;
        return TLPeer();
    }
    TLPeer result;
    switch (p.type) {
    case Peer::User:
        result.tlType = TLValue::PeerUser;
        result.userId = p.id;
        break;
    case Peer::Chat:
        result.tlType = TLValue::PeerChat;
        result.chatId = p.id;
        break;
    case Peer::Channel:
        result.tlType = TLValue::PeerChannel;
        result.channelId = p.id;
        break;
    }
    return result;
}

UserContact AbstractUser::toContact() const
{
    UserContact contact;
    contact.id = id();
    contact.phone = phoneNumber();
    contact.firstName = firstName();
    contact.lastName = lastName();
    return contact;
}

LocalUser::LocalUser(QObject *parent) :
    QObject(parent)
{
}

void LocalUser::setPhoneNumber(const QString &phoneNumber)
{
    m_phoneNumber = phoneNumber;
    if (!m_id) {
        setUserId(qHash(m_phoneNumber));
    }
}

void LocalUser::setFirstName(const QString &firstName)
{
    m_firstName = firstName;
}

void LocalUser::setLastName(const QString &lastName)
{
    m_lastName = lastName;
}

bool LocalUser::isOnline() const
{
    return true;
}

void LocalUser::setDcId(quint32 id)
{
    m_dcId = id;
}

Session *LocalUser::getSession(quint64 authId) const
{
    for (Session *s : m_sessions) {
        if (s->authId == authId) {
            return s;
        }
    }
    return nullptr;
}

QVector<Session *> LocalUser::activeSessions() const
{
    QVector<Session *> result;
    for (Session *s : m_sessions) {
        if (s->isActive()) {
            result.append(s);
        }
    }
    return result;
}

bool LocalUser::hasActiveSession() const
{
    for (Session *s : m_sessions) {
        if (s->isActive()) {
            return true;
        }
    }
    return false;
}

void LocalUser::addSession(Session *session)
{
    m_sessions.append(session);
    session->setUser(this);
    emit sessionAdded(session);
}

void LocalUser::setPlainPassword(const QString &password)
{
    if (password.isEmpty()) {
        m_passwordSalt.clear();
        m_passwordHash.clear();
        return;
    }
    QByteArray pwdSalt(8, Qt::Uninitialized);
    RandomGenerator::instance()->generate(&pwdSalt);
    const QByteArray pwdData = pwdSalt + password.toUtf8() + pwdSalt;
    const QByteArray pwdHash = QCryptographicHash::hash(pwdData, QCryptographicHash::Sha256);
    setPassword(pwdSalt, pwdHash);
}

void LocalUser::setPassword(const QByteArray &salt, const QByteArray &hash)
{
    m_passwordSalt = salt;
    m_passwordHash = hash;
}

TLVector<TLMessage> LocalUser::getHistory(const Peer &peer,
                                     quint32 offsetId,
                                     quint32 offsetDate,
                                     quint32 addOffset,
                                     quint32 limit,
                                     quint32 maxId,
                                     quint32 minId,
                                     quint32 hash) const
{
    if (offsetId || offsetDate || addOffset || minId || maxId || hash) {
        qWarning() << Q_FUNC_INFO << "Unsupported request";
        return {};
    }

    const int actualLimit = qMin<quint32>(limit, 30);

    TLVector<TLMessage> result;
    result.reserve(actualLimit);
    for (int messageId = m_box.lastMessageId(); messageId >= 0; --messageId) {
        const TLMessage *message = m_box.getMessage(messageId);
        if (!message) {
            continue;
        }
        const Telegram::Peer messageDialogPeer = Telegram::Utils::getMessageDialogPeer(*message, userId());
        if (peer.isValid()) {
            if (peer != messageDialogPeer) {
                continue;
            }
        }
        result.append(*message);
        // Messages to self are not marked as Out
        if ((message->fromId == userId()) && (messageDialogPeer != toPeer())) {
            result.last().flags |= TLMessage::Out;
        }
    }

    return result;
}

void LocalUser::importContact(const UserContact &contact)
{
    // Check for contact registration status and the contact id setup performed out of this function
    m_importedContacts.append(contact);

    if (contact.id) {
        m_contactList.append(contact.id);
    }
}

UserDialog *LocalUser::ensureDialog(const Telegram::Peer &peer)
{
    for (int i = 0; i < m_dialogs.count(); ++i) {
        if (m_dialogs.at(i)->peer == peer) {
            return m_dialogs[i];
        }
    }
    m_dialogs.append(new UserDialog());
    m_dialogs.last()->peer = peer;
    return m_dialogs.last();
}

void LocalUser::syncDialog(const Peer &peer, quint32 messageId)
{
    UserDialog *dialog = ensureDialog(peer);
    dialog->topMessage = messageId;
}

void LocalUser::setUserId(quint32 userId)
{
    m_id = userId;
    m_box.setUserId(m_id);
}

} // Server namespace

} // Telegram namespace
