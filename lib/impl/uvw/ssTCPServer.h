#pragma once

#include <memory>
#include <string_view>
#include <uvw/tcp.h>

#include "core/ssEmitter.h"
#include "interface/ssServer.h"

namespace fbrpc::uvwDetail
{
    class sTCPServer final : public sServer
    {
    public:
        sTCPServer();
        ~sTCPServer();

        void bind(std::string_view address, unsigned int port);
        void listen() override;
    private:
        std::shared_ptr<uvw::TCPHandle> m_handle;
    };
}
