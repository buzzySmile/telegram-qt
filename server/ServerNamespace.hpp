#ifndef TELEGRAM_SERVER_NAMESPACE_HPP
#define TELEGRAM_SERVER_NAMESPACE_HPP

#include "TelegramNamespace.hpp"

#include <QHash>
#include <QString>

namespace Telegram {

namespace Server {

struct InputPeer : public Telegram::Peer
{
    InputPeer() = default;
    InputPeer(const Telegram::Peer &peer);

    qint64 accessHash = 0;
};

struct ServerSalt
{
    quint64 salt = 0;
    quint32 validSince = 0;
    quint32 validUntil = 0;
};

struct UserDialog
{
    Telegram::Peer peer;
    QString draftText;
    quint32 topMessage = 0;
    quint32 readInboxMaxId = 0;
    quint32 readOutboxMaxId = 0;
    quint32 unreadCount = 0;
    quint32 unreadMentionsCount = 0;
    quint32 pts = 0;
};

struct UserContact
{
    quint32 id = 0;
    QString phone;
    QString firstName;
    QString lastName;
};

class MessageData
{
public:
    MessageData() = default;
    MessageData(quint32 from, Peer to, const QString &text);

    quint64 globalId() const { return m_globalId; }
    QString text() const { return m_text; }
    Peer toPeer() const { return m_to; }
    quint32 fromId() const { return m_fromId; }
    quint32 date() const { return m_date; }

    void addReference(const Peer &peer, quint32 messageId);
    quint32 getReference(const Peer &peer) const { return m_references.value(peer); }
    void setGlobalId(quint64 id);

protected:
    QHash<Peer, quint32> m_references;
    QString m_text;
    Peer m_to;
    quint64 m_globalId = 0;
    quint32 m_fromId = 0;
    quint32 m_date = 0;
};

} // Server namespace

} // Telegram namespace

#endif // TELEGRAM_SERVER_NAMESPACE_HPP
