#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <type_traits>
#include <memory>

#include "fbrpc/core/ssCommon.h"
#include "fbrpc/core/ssPromise.h"
#include "fbrpc/core/ssUniqueFunction.h"
#include "fbrpc/core/ssBuffer.h"
#include "fbrpc/core/ssFlatBufferHelper.h"

namespace fbrpc
{
	class sService : public sFlatBufferHelper
	{
	public:
		virtual void init() = 0;
		virtual std::string name() const = 0;
		virtual std::size_t hash() const = 0;

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