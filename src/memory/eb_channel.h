#pragma once
#include <type_traits>
#include <atomic>
#include "readerwriterqueue.h"

namespace lfx::bus {

    template<typename T>
    class Channel {
        static_assert(std::is_trivially_copyable_v<T>, 
                      "LFX Error: I Payload del Channel devono essere POD (trivially copyable)!");

    private:
        moodycamel::ReaderWriterQueue<T> m_queue;
        // Contatore atomico per sapere se abbiamo bucato il worst-case
        std::atomic<size_t> m_droppedEventsCount{0};

    public:
        explicit Channel(size_t worstCaseCapacity) 
            : m_queue(worstCaseCapacity) {}

        Channel(const Channel&) = delete;
        Channel& operator=(const Channel&) = delete;

        // --- LATO PRODUTTORE (Single Producer) ---
        
        bool Publish(const T& event) noexcept {
            if (!m_queue.try_enqueue(event)) {
                // Coda satura! Incrementiamo il contatore di drop in modo lock-free
                m_droppedEventsCount.fetch_add(1, std::memory_order_relaxed);
                return false;
            }
            return true;
        }

        // --- LATO CONSUMATORE (Single Consumer) ---

        // Estrae un singolo evento
        bool Consume(T& outEvent) noexcept {
            return m_queue.try_dequeue(outEvent);
        }

        // Svuota tutti gli eventi presenti in questo istante passandoli a una lambda/funzione
        // Esempio d'uso: channel.ConsumeAll([](const TestEventPayload& ev) { ... });
        template<typename Func>
        size_t ConsumeAll(Func&& callback) noexcept {
            size_t consumed = 0;
            T ev;
            while (m_queue.try_dequeue(ev)) {
                callback(ev);
                ++consumed;
            }
            return consumed;
        }

        // --- DIAGNOSTICA / LOGGER ---

        size_t GetApproxSize() const noexcept {
            return m_queue.size_approx();
        }

        size_t GetDroppedCount() const noexcept {
            return m_droppedEventsCount.load(std::memory_order_relaxed);
        }

        void ResetDroppedCount() noexcept {
            m_droppedEventsCount.store(0, std::memory_order_relaxed);
        }
    };

} // namespace lfx::bus