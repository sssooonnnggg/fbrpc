#include <cassert>
#include "ssTCPConnection.h"

namespace fbrpc::uvwDetail
{
	sTCPConnection::sTCPConnection(std::shared_ptr<uvw::TCPHandle> handle)
		: m_handle(handle)
	{
		assert(m_handle);
	}

	sTCPConnection::~sTCPConnection()
	{
		m_handle->clear();
		m_handle->close();
	}

	void sTCPConnection::initDefaultEventHandler()
	{
		m_handle->on<uvw::ErrorEvent>([this](const uvw::ErrorEvent& e, uvw::TCPHandle& handle) { emit(sError{ e.code(), e.what() }); });
		m_handle->on<uvw::CloseEvent>([this](const uvw::CloseEvent&, uvw::TCPHandle&) { m_handle->close(); emit(sCloseEvent{}); });
		m_handle->on<uvw::EndEvent>([this](const uvw::EndEvent&, uvw::TCPHandle& sock) { sock.close(); emit(sEndEvent{}); });

		m_handle->on<uvw::DataEvent>([this](uvw::DataEvent& e, uvw::TCPHandle&) { emit(sDataEvent{ e.data.get(), e.length}); });
	}

	void sTCPConnection::send(std::unique_ptr<char[]> data, std::size_t length)
	{
		m_handle->write(std::move(data), length);
	}

	void sTCPConnection::close()
	{
		m_handle->close();
	}
}