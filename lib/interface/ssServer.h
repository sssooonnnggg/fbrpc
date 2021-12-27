#pragma once

#include <algorithm>
#include <vector>
#include <memory>
#include <type_traits>

#include "core/ssEmitter.h"
#include "ssConnection.h"

namespace fbrpc
{
    class sServer : public sEmitter<sServer>
    {
    public:
        virtual void listen() = 0;

    protected:
        void addClient(std::shared_ptr<sConnection> client);
        void removeClient(std::shared_ptr<sConnection> client);

        template <class T, typename A = std::enable_if_t<std::is_base_of_v<sConnection, T>>>
        void addClient(std::shared_ptr<T> client) { addClient(std::static_pointer_cast<sConnection>(client)); }

        template <class T, typename A = std::enable_if_t<std::is_base_of_v<sConnection, T>>>
        void removeClient(std::shared_ptr<T> client) { removeClient(std::static_pointer_cast<sConnection>(client)); }

        int generateClientId();

    private:
        std::vector<std::shared_ptr<sConnection>> m_clients;
    };
}