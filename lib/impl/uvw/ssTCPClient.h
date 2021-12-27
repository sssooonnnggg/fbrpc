#pragma once

#include "interface/ssClient.h"
#include "ssUvLoop.h"

namespace fbrpc::uvwDetail
{
	class sTCPClient final : public sClient
	{
	public:
		sTCPClient();
		~sTCPClient();
		void setServerAddress(std::string_view address, unsigned int port);
		void connect() override;
	private:
		std::string m_address;
		unsigned int m_port = 0;
		std::shared_ptr<uvw::TCPHandle> m_handle;
	};
}