#pragma once

#include <cassert>
#include <memory>
#include <optional>

#include "flatbuffers/flatbuffers.h"

namespace fbrpc
{
	struct sBufferView
	{
		template <class T> T read(std::size_t offset = 0) const
		{
			assert(length >= offset + sizeof(T));
			return *reinterpret_cast<const T*>(data + offset);
		}

		sBufferView view(std::size_t start = 0)
		{
			assert(start < length);
			return sBufferView{ data + start, length - start };
		}

		sBufferView view(std::size_t start, std::size_t size)
		{
			assert(start < length && start + size <= length);
			return sBufferView{ data + start, size };
		}

		const char* data;
		std::size_t length;
	};

	struct sBuffer
	{
		static sBuffer clone(const sBuffer& src)
		{
			return clone(src.data.get(), src.length);
		}

		static sBuffer clone(const sBuffer& src, std::size_t offset, std::size_t size)
		{
			assert(offset + size <= src.length);
			return clone(src.data.get() + offset, size);
		}

		static sBuffer clone(const flatbuffers::FlatBufferBuilder& builder)
		{
			return sBuffer::clone(
				reinterpret_cast<const char*>(builder.GetBufferPointer()),
				static_cast<std::size_t>(builder.GetSize()));
		}

		static sBuffer clone(const char* data, std::size_t length)
		{
			auto buffer = std::make_unique<char[]>(length);
			std::memcpy(buffer.get(), data, length);
			return sBuffer{ std::move(buffer), length };
		}

		static sBuffer from(char* data, std::size_t length)
		{
			return sBuffer{ std::unique_ptr<char[]>(data), length };
		}

		void append(const sBuffer& buffer)
		{
			append(buffer.data.get(), buffer.length);
		}

		template<class T>
		void append(T value)
		{
			append(reinterpret_cast<const char*>(&value), sizeof(T));
		}

		void append(const flatbuffers::FlatBufferBuilder& buffer)
		{
			append(reinterpret_cast<const char*>(buffer.GetBufferPointer()), buffer.GetSize());
		}

		void append(const char* appendData, std::size_t appendSize)
		{
			auto newData = std::make_unique<char[]>(length + appendSize);
			memcpy(newData.get(), data.get(), length);
			memcpy(newData.get() + length, appendData, appendSize);
			length += appendSize;
			data = std::move(newData);
		}

		void consume(std::size_t size)
		{
			assert(length >= size);
			if (length == size)
			{
				data.reset();
				length = 0;
			}
			else
			{
				auto newData = std::make_unique<char[]>(length - size);
				memcpy(newData.get(), data.get() + size, length - size);
				length -= size;
				data = std::move(newData);
			}
		}

		sBufferView view(std::size_t start = 0)
		{
			assert(start < length);
			return sBufferView{ data.get() + start, length - start};
		}

		sBufferView view(std::size_t start, std::size_t size)
		{
			assert(start < length && start + size <= length);
			return sBufferView{ data.get() + start, size };
		}

		bool empty() const { return length == 0; }

		std::unique_ptr<char[]> data;
		std::size_t length = 0;
	};
}