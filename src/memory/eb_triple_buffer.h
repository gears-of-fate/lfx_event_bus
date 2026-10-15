#pragma once
#include <atomic>
#include <type_traits>
#include <memory>

namespace lfx::bus {

    template<typename T>
    class TripleBuffer {
        static_assert(std::is_trivially_copyable_v<T>, 
                      "LFX Error: Il Payload del TripleBuffer deve essere un POD!");

    private:
        struct alignas(64) AlignedBuffer {
            T data{};
        };

        std::unique_ptr<AlignedBuffer[]> m_buffers;

        uint8_t m_writeIndex{0};
        uint8_t m_readIndex{2};

        // Bit 0..1 = indice del middle buffer (0, 1 o 2)
        // Bit 7 (0x80) = flag "Dirty" (1 = nuovo stato pronto, 0 = già letto)
        alignas(64) std::atomic<uint8_t> m_sharedState{1};

    public:
        TripleBuffer() : m_buffers(std::make_unique<AlignedBuffer[]>(3)) {}

        TripleBuffer(const TripleBuffer&) = delete;
        TripleBuffer& operator=(const TripleBuffer&) = delete;

        // --- LATO PRODUTTORE (Single Producer - es. ECS) ---

        // Accesso diretto al buffer di scrittura
        T& GetBackBuffer() noexcept {
            return m_buffers[m_writeIndex].data;
        }

        // Pubblica il buffer appena scritto scambiandolo col Middle Buffer
        void Commit() noexcept {
            uint8_t newShared = m_writeIndex | 0x80;
            uint8_t prevShared = m_sharedState.exchange(newShared, std::memory_order_acq_rel);
            m_writeIndex = prevShared & 0x03;
        }

        // Scorciatoia comoda se hai già una struct pronta e vuoi copiarla e pubblicarla al volo
        void WriteAndCommit(const T& newState) noexcept {
            m_buffers[m_writeIndex].data = newState;
            Commit();
        }

        // --- LATO CONSUMATORE (Single Consumer - es. Render / MIDI LED) ---

        // Ritorna true se il produttore ha fatto almeno un Commit() dall'ultima lettura
        bool HasNewState() const noexcept {
            return (m_sharedState.load(std::memory_order_relaxed) & 0x80) != 0;
        }

        // Restituisce SEMPRE l'ultimo stato valido (utile per il Rendering a ogni frame).
        // Se c'è un nuovo buffer nel mezzo, fa lo swap automaticamente.
        const T& AcquireFrontBuffer() noexcept {
            uint8_t currentShared = m_sharedState.load(std::memory_order_relaxed);
            if (currentShared & 0x80) {
                uint8_t prevShared = m_sharedState.exchange(m_readIndex, std::memory_order_acq_rel);
                m_readIndex = prevShared & 0x03;
            }
            return m_buffers[m_readIndex].data;
        }

        // Fa lo swap e restituisce il puntatore SOLO SE c'è uno stato nuovo (altrimenti nullptr).
        // Perfetto per il Launchpad o l'Audio: aggiorni l'hardware solo quando lo stato è cambiato!
        const T* TryAcquireNewFrontBuffer() noexcept {
            uint8_t currentShared = m_sharedState.load(std::memory_order_relaxed);
            if (currentShared & 0x80) {
                uint8_t prevShared = m_sharedState.exchange(m_readIndex, std::memory_order_acq_rel);
                m_readIndex = prevShared & 0x03;
                return &m_buffers[m_readIndex].data;
            }
            return nullptr;
        }
    };

} // namespace lfx::bus