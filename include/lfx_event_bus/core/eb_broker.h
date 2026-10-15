#pragma once
#include <memory>
#include <functional>
#include "eb_config.h"
#include "../payloads/eb_payloads.h"

namespace lfx::bus {

    // Forward declarations: nascondiamo completamente l'implementazione e Moodycamel!
    template<typename T> class Channel;
    template<typename T> class TripleBuffer;

    class Broker {
    private:
        BusConfig m_config;
        std::unique_ptr<Channel<TestEventPayload>> m_eventChannel;
        std::unique_ptr<TripleBuffer<LfxRenderable>> m_stateBuffer;

    public:
        explicit Broker(const BusConfig& config);
        ~Broker(); // Obbligatorio definirlo nel .cpp quando si usano unique_ptr con forward decl

        Broker(const Broker&) = delete;
        Broker& operator=(const Broker&) = delete;

        // --- API EVENTI (Coda Circolare) ---
        bool PublishEvent(const TestEventPayload& ev) noexcept;
        bool ConsumeEvent(TestEventPayload& outEv) noexcept;
        size_t ConsumeAllEvents(const std::function<void(const TestEventPayload&)>& callback) noexcept;
        size_t GetDroppedEventsCount() const noexcept;

        // --- API STATI (Triple Buffer) ---
        LfxRenderable* GetStateBackBuffer() noexcept;
        void CommitState() noexcept;
        void PublishState(const LfxRenderable& state) noexcept;

        bool HasNewState() const noexcept;
        const LfxRenderable* AcquireStateFrontBuffer() noexcept;
        const LfxRenderable* TryAcquireNewStateFrontBuffer() noexcept;

        const BusConfig& GetConfig() const noexcept { return m_config; }
    };

} // namespace lfx::bus