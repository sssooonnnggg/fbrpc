#include "ssServer.h"

namespace fbrpc
{
    void sServer::addClient(std::shared_ptr<sConnection> client)
    {
        client->setId(generateClientId());
        m_clients.push_back(client);
    }

    void sServer::removeClient(std::shared_ptr<sConnection> client)
    {
        auto it = std::find(m_clients.begin(), m_clients.end(), client);
        if (it != m_clients.end())
            m_clients.erase(it);
    }

    int sServer::generateClientId()
    {
        static auto clientId = -1;
        return ++clientId;
    }
}