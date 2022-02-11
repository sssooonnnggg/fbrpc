#include <cassert>
#include <uvw/tcp.h>

#include "fbrpc/core/ssError.h"

#include "ssUvLoop.h"
#include "ssTCPConnection.h"
#include "ssTCPServer.h"

namespace fbrpc::uvwDetail
{
    sTCPServer::sTCPServer()
    {
        m_handle = sUvLoop::allocHandle([this](const uvw::ErrorEvent& e, uvw::TCPHandle& handle) 
            { 
                emit(sError{ e.code(), e.what() }); 
            });
    }

    sTCPServer::~sTCPServer()
    {
        if (m_handle)
        {
            m_handle->clear();
            m_handle->close();
        }
    }

    void sTCPServer::bind(std::string_view address, unsigned int port)
    {
        m_address = address;
        m_port = port;
    }

    void sTCPServer::listen()
    {
        auto onConnection = [this](const uvw::ListenEvent&, uvw::TCPHandle& handle)
        {
            std::shared_ptr<uvw::TCPHandle> clientHandle = handle.loop().resource<uvw::TCPHandle>();
            auto client = std::make_shared<sTCPConnection>(clientHandle);

            addClient(client);

            client->initDefaultEventHandler();
            handle.accept(*clientHandle);

            clientHandle->read();
            emit(sConnectionEvent{ client });
        };

        if (m_handle)
        {
            m_handle->on<uvw::ListenEvent>(onConnection);
            m_handle->bind(m_address, m_port);
            m_handle->listen();
        }
    }
}