#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include "lfx_event_bus/LfxEventBus.h"

int main() {
    lfx::bus::BusConfig config;
    config.enableEventChannel = true;
    config.enableStateBuffer  = true;
    config.eventQueueCapacity = 64;

    lfx::bus::Broker broker(config);
    std::atomic<bool> isRunning{true};

    // Thread Consumatore (Simula Hardware Gate: Filament / Audio)
    std::thread consumerThread([&]() {
        while (isRunning.load(std::memory_order_relaxed)) {
            // Svuota la coda circolare degli eventi
            broker.ConsumeAllEvents([](const lfx::bus::TestEventPayload& ev) {
                std::cout << "[Consumer] Evento ricevuto! ID: " << ev.eventId
                          << " | Val: " << ev.value << "\n";
            });

            // Acquisisce lo stato dal Triple Buffer solo se nuovo
            if (const auto* newState = broker.TryAcquireNewStateFrontBuffer()) {
                std::cout << "[Consumer] Nuovo Stato! GraphicID: " << newState->graphicId
                          << " | Alpha: " << newState->alpha << "\n";
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    // Thread Produttore (Simula Logica ECS)
    for (uint32_t frame = 1; frame <= 5; ++frame) {
        broker.PublishEvent({ .eventId = frame, .value = frame * 10.5f });

        if (auto* back = broker.GetStateBackBuffer()) {
            back->graphicId = 100 + frame;
            back->alpha     = 1.0f / static_cast<float>(frame);
            broker.CommitState();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    isRunning.store(false);
    consumerThread.join();

    std::cout << "Eventi persi per overflow: " << broker.GetDroppedEventsCount() << "\n";
    return 0;
}