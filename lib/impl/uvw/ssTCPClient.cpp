#include <cassert>

#include "core/ssError.h"
#include "ssTCPConnection.h"
#include "ssTCPClient.h"

namespace fbrpc::uvwDetail
{
	sTCPClient::sTCPClient()
	{
		m_handle = sUvLoop::allocHandle([this](const uvw::ErrorEvent& e, uvw::TCPHandle& handle) { emit(sError{ e.code(), e.what() }); });
	}

	sTCPClient::~sTCPClient()
	{
		if (m_handle)
			m_handle->close();
	}

	void sTCPClient::setServerAddress(std::string_view address, unsigned int port)
	{
		m_address = address;
		m_port = port;
	}

	void sTCPClient::connect()
	{
		assert(m_handle);
		m_handle->on<uvw::ConnectEvent>(
			[this](const uvw::ConnectEvent& e, const uvw::TCPHandle&)
			{
				auto newConnection = std::make_shared<sTCPConnection>(m_handle);
				newConnection->init();
				m_handle->read();
				emit(sConnectionEvent{ newConnection });
			}
		);
		m_handle->connect(m_address, m_port);
	}
}