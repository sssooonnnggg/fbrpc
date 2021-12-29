#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

namespace fbrpc
{
    template <class Event>
    using sListener = std::function<void(const Event&)>;

    template <class T>
    class sEmitter
    {
    public:

        virtual ~sEmitter() = default;

        struct sSlot
        {
            std::size_t id = -1;
        };

        class sBaseHandler
        {
        public:
            virtual ~sBaseHandler() = default;
        };

        template <class Event>
        class sHandler : public sBaseHandler
        {
        public:

            struct sListenerContext
            {
                sSlot               slot;
                sListener<Event>    listener;
            };

            sSlot on(sListener<Event> listener)
            {
                auto slot = createSlot();
                m_listeners.push_back(sListenerContext{ slot, std::move(listener) });
                return slot;
            }

            void off(const sSlot& slot)
            {
                auto it = std::find_if(m_listeners.being(), m_listeners.end(). [](auto&& ctx) { return ctx->slot == slot });
                if (it != m_listeners.end())
                    m_listeners.erase(it);
            }

            void emit(const Event& event)
            {
                for (auto&& ctx : m_listeners)
                    ctx.listener(event);
            }

        private:
            sSlot createSlot() { return sSlot{ ++m_slotId }; }

        private:
            std::vector<sListenerContext>   m_listeners;
            std::size_t                     m_slotId = -1;
        };

        template <class Event>
        sSlot on(sListener<Event> listener)
        {
            return handler<Event>().on(std::move(listener));
        }

        void off(const sSlot& slot)
        {
            handler<Event>().off(slot);
        }

    protected:

        template <class Event>
        void emit(const Event& event)
        {
            handler<Event>().emit(event);
        }

    private:

        template <class Event>
        sHandler<Event>& handler()
        {
            auto eventType = getEventType<Event>();
            if (eventType >= m_handlers.size())
                m_handlers.resize(eventType + 1);

            if (!m_handlers[eventType])
                m_handlers[eventType] = std::make_unique<sHandler<Event>>();

            return static_cast<sHandler<Event>&>(*m_handlers[eventType]);
        }

        std::size_t eventType()
        {
            static std::size_t type = -1;
            return ++type;
        }

        template <class>
        std::size_t getEventType()
        {
            static std::size_t type = eventType();
            return type;
        }

    private:
        std::vector<std::unique_ptr<sBaseHandler>> m_handlers;
    };
}

