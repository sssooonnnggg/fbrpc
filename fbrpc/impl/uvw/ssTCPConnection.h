#pragma once

#include <uvw.hpp>

#include "fbrpc/interface/ssConnection.h"

#include "fbrpc/core/ssEmitter.h"
#include "fbrpc/core/ssError.h"

namespace fbrpc::uvwDetail
{
	class sTCPConnection final : public sConnection
	{
	public:
		sTCPConnection(std::shared_ptr<uvw::TCPHandle> handle);
		~sTCPConnection();

		void init();

		void send(std::unique_ptr<char[]> data, std::size_t length) override;
		void close() override;
	private:
		std::shared_ptr<uvw::TCPHandle> m_handle;
	};
}