#include <uvw/loop.h>
#include <cassert>

#include "fbrpc/core/ssError.h"
#include "ssUvLoop.h"

namespace fbrpc::uvwDetail
{
    std::shared_ptr<uvw::Loop> sUvLoop::getUvLoop()
    {
        return uvw::Loop::getDefault();
    }

    std::shared_ptr<uvw::TCPHandle> sUvLoop::allocHandle(OnError errorHandler)
    {
        auto handle = getUvLoop()->resource<uvw::TCPHandle>();
        assert(handle);

        handle->on<uvw::ErrorEvent>(std::move(errorHandler));
        return handle;
    }
}