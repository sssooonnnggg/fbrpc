#pragma once

#include <cstddef>
#include <memory>

#include "fbrpc/core/ssBuffer.h"
#include "fbrpc/core/ssEmitter.h"

namespace fbrpc
{
    struct sDataEvent
    {
        const char* data;
        std::size_t length;
    };  

    // close connection manually
    struct sCloseEvent {};

    // closed passively
    struct sEndEvent {};

    class sConnection : public sEmitter<sConnection>
    {
    public:
        void setId(int id) { m_id = id; }
        int getId() const { return m_id; }

        void send(const std::string& buffer) 
        { 
            auto data = std::make_unique<char[]>(buffer.length());
            memcpy(data.get(), buffer.c_str(), buffer.length());
            send(std::move(data), buffer.length());
        }

        virtual void send(std::unique_ptr<char[]> data, std::size_t length) = 0;
        virtual void send(sBuffer&& buffer) = 0;
        virtual void close() = 0;
    private:
        int m_id = -1;
    };

    // receive a new connection
    struct sConnectionEvent
    {
        std::weak_ptr<sConnection> connection;
    };

}