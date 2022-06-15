#pragma once

#include "fbrpc/core/ssBuffer.h"

namespace fbrpc
{
	//
	// Package format
	// | package size (8 bytes) | request id (8 bytes) | service hash (8 bytes) | api hash (8 bytes) | flat buffer |
	//

	static_assert(sizeof(std::size_t) == 8, "std::size_t size mismatch!");

	enum ePackageOffset
	{
		kPackageSize = 0,
		kRequestId = 8,
		kServiceHash = 16,
		kApiHash = 24,
		kFlatBuffer = 32
	};

	class sPackageReader
	{
	public:

		sPackageReader(sBufferView buffer)
			: m_buffer(buffer)
		{}

		bool ready() const 
		{ 
			if (m_buffer.length < ePackageOffset::kRequestId)
				return false;

			return packageSize() <= m_buffer.length;
		}

		std::size_t packageSize() const
		{
			return m_buffer.read<std::size_t>(ePackageOffset::kPackageSize);
		}

		std::size_t requestId() const
		{
			return m_buffer.read<std::size_t>(ePackageOffset::kRequestId);
		}

		std::size_t serviceHash() const
		{
			return m_buffer.read<std::size_t>(ePackageOffset::kServiceHash);
		}

		std::size_t apiHash() const
		{
			return m_buffer.read<std::size_t>(ePackageOffset::kApiHash);
		}

	private:
		sBufferView m_buffer;
	};

	class sPackageWritter
	{
	public:

		sPackageWritter()
		{
			static std::size_t packageSizePlaceHolder = 0;
			write(packageSizePlaceHolder);
		}

		template <class T>
		sPackageWritter& write(T&& value) &
		{
			m_buffer.append(std::forward<T>(value));
			return *this;
		}

		template <class T>
		sPackageWritter&& write(T&& value) &&
		{
			m_buffer.append(std::forward<T>(value));
			return std::move(*this);
		}

		sBuffer getBuffer() &
		{
			finish();
			return sBuffer::clone(m_buffer);
		}

		sBuffer getBuffer() &&
		{
			finish();
			return std::move(m_buffer);
		}

	private:
		void finish()
		{
			*(reinterpret_cast<std::size_t*>(m_buffer.data())) = m_buffer.length();
		}

	private:
		sBuffer m_buffer;
	};
}