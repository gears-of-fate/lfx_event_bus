#include "lfx_event_bus/core/eb_broker.h"
#include "memory/eb_channel.h"
#include "memory/eb_triple_buffer.h"

namespace lfx::bus {

    Broker::Broker(const BusConfig& config)
        : m_config(config)
    {
        if (m_config.enableEventChannel) {
            m_eventChannel = std::make_unique<Channel<TestEventPayload>>(m_config.eventQueueCapacity);
        }

        if (m_config.enableStateBuffer) {
            m_stateBuffer = std::make_unique<TripleBuffer<LfxRenderable>>();
        }
    }

    // Il distruttore qui nel .cpp vede la definizione completa di Channel e TripleBuffer
    Broker::~Broker() = default;

    // --- EVENTI ---
    bool Broker::PublishEvent(const TestEventPayload& ev) noexcept {
        return m_eventChannel ? m_eventChannel->Publish(ev) : false;
    }

    bool Broker::ConsumeEvent(TestEventPayload& outEv) noexcept {
        return m_eventChannel ? m_eventChannel->Consume(outEv) : false;
    }

    size_t Broker::ConsumeAllEvents(const std::function<void(const TestEventPayload&)>& callback) noexcept {
        return m_eventChannel ? m_eventChannel->ConsumeAll(callback) : 0;
    }

    size_t Broker::GetDroppedEventsCount() const noexcept {
        return m_eventChannel ? m_eventChannel->GetDroppedCount() : 0;
    }

    // --- STATI ---
    LfxRenderable* Broker::GetStateBackBuffer() noexcept {
        return m_stateBuffer ? &m_stateBuffer->GetBackBuffer() : nullptr;
    }

    void Broker::CommitState() noexcept {
        if (m_stateBuffer) m_stateBuffer->Commit();
    }

    void Broker::PublishState(const LfxRenderable& state) noexcept {
        if (m_stateBuffer) m_stateBuffer->WriteAndCommit(state);
    }

    bool Broker::HasNewState() const noexcept {
        return m_stateBuffer ? m_stateBuffer->HasNewState() : false;
    }

    const LfxRenderable* Broker::AcquireStateFrontBuffer() noexcept {
        return m_stateBuffer ? &m_stateBuffer->AcquireFrontBuffer() : nullptr;
    }

    const LfxRenderable* Broker::TryAcquireNewStateFrontBuffer() noexcept {
        return m_stateBuffer ? m_stateBuffer->TryAcquireNewFrontBuffer() : nullptr;
    }

} // namespace lfx::bus