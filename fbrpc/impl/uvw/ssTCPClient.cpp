#include <cassert>

#include "fbrpc/core/ssError.h"
#include "ssTCPConnection.h"
#include "ssTCPClient.h"

namespace fbrpc::uvwDetail
{
	sTCPClient::sTCPClient()
	{
		m_handle = sUvLoop::allocHandle(
			[this](const uvw::ErrorEvent& e, uvw::TCPHandle& handle) 
			{ 
				emit(sError{ e.code(), e.what() });
				if (e.code() == UV_ECONNRESET)
					m_handle->close();
			}
		);

		m_handle->on<uvw::CloseEvent>([this](const uvw::CloseEvent&, uvw::TCPHandle&) { m_handle->close(); emit(sCloseEvent{}); });
		m_handle->on<uvw::EndEvent>([this](const uvw::EndEvent&, uvw::TCPHandle& sock) { sock.close(); emit(sEndEvent{}); });
		m_handle->on<uvw::DataEvent>([this](uvw::DataEvent& e, uvw::TCPHandle&) { emit(sDataEvent{ e.data.get(), e.length }); });

		m_handle->on<uvw::ConnectEvent>(
			[this](const uvw::ConnectEvent& e, const uvw::TCPHandle&)
			{
				auto newConnection = std::make_shared<sTCPConnection>(m_handle);
				m_handle->read();
				emit(sConnectionEvent{ newConnection });
			}
		);
	}

	sTCPClient::~sTCPClient()
	{

	}

	void sTCPClient::setServerAddress(std::string_view address, unsigned int port)
	{
		m_address = address;
		m_port = port;
	}

	void sTCPClient::connect()
	{
		m_handle->connect(m_address, m_port);
	}
}