#pragma once

#include <cassert>
#include <deque>
#include <iterator>
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
			return clone(src.data(), src.length());
		}

		static sBuffer clone(const sBuffer& src, std::size_t offset, std::size_t size)
		{
			assert(offset + size <= src.length());
			return clone(src.data() + offset, size);
		}

		static sBuffer clone(const flatbuffers::FlatBufferBuilder& builder)
		{
			return sBuffer::clone(
				reinterpret_cast<const char*>(builder.GetBufferPointer()),
				static_cast<std::size_t>(builder.GetSize()));
		}

		static sBuffer clone(const char* data, std::size_t length)
		{
			std::vector<char> buffer(length);
			std::copy(data, data + length, buffer.data());
			return sBuffer{ 0, std::move(buffer) };
		}

		sBuffer(std::size_t s = 0, std::vector<char> buf = {})
			: start(s), buffer(std::move(buf)) {}
		sBuffer(const sBuffer& buffer) = default;
		sBuffer(sBuffer&& buf)
			: start(buf.start), buffer(std::move(buf.buffer)) {}

		void append(const sBuffer& input)
		{
			buffer.insert(buffer.end(), input.buffer.begin() + input.start, input.buffer.end());
		}

		void append(sBuffer&& input)
		{
			buffer.insert(buffer.end(), std::make_move_iterator(input.buffer.begin() + input.start), std::make_move_iterator(input.buffer.end()));
		}

		template<class T>
		void append(T value)
		{
			append(reinterpret_cast<const char*>(&value), sizeof(T));
		}

		void append(const flatbuffers::FlatBufferBuilder& fb)
		{
			append(reinterpret_cast<const char*>(fb.GetBufferPointer()), fb.GetSize());
		}

		void append(const char* appendData, std::size_t appendSize)
		{
			auto oldSize = buffer.size();
			buffer.resize(oldSize + appendSize);
			std::copy(appendData, appendData + appendSize, buffer.data() + oldSize);
		}

		void consume(std::size_t size)
		{
			assert(length() >= size);
			if (length() == size)
			{
				start = 0;
				buffer.clear();
			}
			else
			{
				start += size;
			}
		}

		sBufferView view(std::size_t offset = 0)
		{
			assert(offset < length());
			return sBufferView{ data() + offset, length() - offset};
		}

		sBufferView view(std::size_t offset, std::size_t size)
		{
			assert(offset < length() && offset + size <= length());
			return sBufferView{ data() + offset, size };
		}

		bool empty() const { return length() == 0; }
		void clear() { buffer.clear(); }
		const char* data() const { return buffer.data() + start; }
		char* data() { return &(buffer[0]) + start; }
		std::size_t length() const { return buffer.size() - start; }

		std::size_t			start;
		std::vector<char> 	buffer;
	};
}