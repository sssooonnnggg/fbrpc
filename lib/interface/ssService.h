#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <type_traits>
#include <memory>

#include "core/ssPromise.h"
#include "core/ssUniqueFunction.h"
#include "core/ssBuffer.h"
#include "core/ssFlatBufferHelper.h"

namespace fbrpc
{
	class sService : public sFlatBufferHelper
	{
	public:
		virtual void init() = 0;
		virtual std::string name() const = 0;
		std::size_t hash();

		using sResponder = sUniqueFunction<void(sBuffer)>;
		void processBuffer(sBufferView buffer, sResponder responder);
		
	protected:

		template <class T> std::unique_ptr<sPromise<T>> createPromise() { return std::unique_ptr<sPromise<T>>(new sPromise<T>()); }

		using sApiWrapper = std::function<void(sBufferView, sResponder)>;
		void addApiWrapper(std::size_t hash, sApiWrapper wrapper);

	private:
		std::unordered_map<std::size_t, sApiWrapper> m_apiWrappers;
	};
}