#include "fbrpc/impl/uvw/ssUvLoop.h"

#include "ssTimer.h"

namespace fbrpc
{
	namespace uvwDetail
	{
		class sUvTimer : public sTimer
		{
		public:
			sUvTimer()
			{
				m_handle = sUvLoop::getUvLoop()->resource<uvw::TimerHandle>();
			}

			~sUvTimer() override
			{
				if (m_handle)
					m_handle->close();
			}

			void once(int timeoutMs, std::function<void()> onTimeout) override
			{
				timeout(timeoutMs, 0, std::move(onTimeout));
			}

			void repeat(int timeoutMs, std::function<void()> onTimeout) override
			{
				timeout(timeoutMs, timeoutMs, std::move(onTimeout));
			}

			void timeout(int timeoutMs, int repeatMs, std::function<void()> onTimeout)
			{
				if (m_handle)
				{
					m_handle->on<uvw::TimerEvent>([onTimeout](const auto&, auto&) { onTimeout(); });
					m_handle->start(uvw::TimerHandle::Time{ timeoutMs }, uvw::TimerHandle::Time{ repeatMs });
				}
			}

			void stop() override
			{
				if (m_handle)
					m_handle->stop();
			}
		private:
			std::shared_ptr<uvw::TimerHandle> m_handle;
		};
	}

	std::unique_ptr<sTimer> sTimer::create()
	{
		return std::make_unique<uvwDetail::sUvTimer>();
	}
}