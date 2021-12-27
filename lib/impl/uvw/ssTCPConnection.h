#pragma once

#include <uvw.hpp>

#include "interface/ssConnection.h"

#include "core/ssEmitter.h"
#include "core/ssError.h"

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