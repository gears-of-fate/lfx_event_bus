#include <gtest/gtest.h>
#include "lfx_event_bus/LfxEventBus.h"

using namespace lfx::bus;

// 1. Test base: Pubblicazione e Consumo Eventi
TEST(EventBusTest, ChannelPublishAndConsume) {
    BusConfig cfg;
    Broker broker(cfg);

    TestEventPayload inEvent{ .eventId = 42, .value = 3.14f };
    ASSERT_TRUE(broker.PublishEvent(inEvent));

    TestEventPayload outEvent{};
    ASSERT_TRUE(broker.ConsumeEvent(outEvent));
    EXPECT_EQ(outEvent.eventId, 42);
    EXPECT_FLOAT_EQ(outEvent.value, 3.14f);
}

// 2. Test base: Scambio e Dirty Bit nel Triple Buffer
TEST(EventBusTest, TripleBufferSwapAndDirtyFlag) {
    BusConfig cfg;
    Broker broker(cfg);

    EXPECT_FALSE(broker.HasNewState());

    LfxRenderable* back = broker.GetStateBackBuffer();
    ASSERT_NE(back, nullptr);
    back->graphicId = 99;
    back->alpha = 0.5f;
    broker.CommitState();

    EXPECT_TRUE(broker.HasNewState());

    const LfxRenderable* front = broker.TryAcquireNewStateFrontBuffer();
    ASSERT_NE(front, nullptr);
    EXPECT_EQ(front->graphicId, 99);
    EXPECT_FLOAT_EQ(front->alpha, 0.5f);

    // Dopo la lettura, il flag Dirty deve essersi abbassato
    EXPECT_FALSE(broker.HasNewState());
    EXPECT_EQ(broker.TryAcquireNewStateFrontBuffer(), nullptr);
}

// 3. Test Frame Skipping: Il Triple Buffer consegna sempre l'ultimo stato
TEST(EventBusTest, TripleBufferFrameSkipping) {
    BusConfig cfg;
    Broker broker(cfg);

    // L'ECS produce 3 frame velocissimi mentre la GPU è occupata
    for (uint32_t i = 1; i <= 3; ++i) {
        LfxRenderable* back = broker.GetStateBackBuffer();
        back->graphicId = i * 10; // 10, 20, 30
        broker.CommitState();
    }

    // Quando la GPU finalmente legge, deve trovare direttamente il frame 3 (30)
    const LfxRenderable* front = broker.TryAcquireNewStateFrontBuffer();
    ASSERT_NE(front, nullptr);
    EXPECT_EQ(front->graphicId, 30);
}

// 4. Test Modularità Config: Nessun crash se i motori sono spenti
TEST(EventBusTest, DisabledModulesAreNullSafe) {
    BusConfig cfg;
    cfg.enableEventChannel = false;
    cfg.enableStateBuffer  = false;

    Broker broker(cfg);

    TestEventPayload ev{ 1, 1.0f };
    EXPECT_FALSE(broker.PublishEvent(ev));
    EXPECT_FALSE(broker.ConsumeEvent(ev));
    EXPECT_EQ(broker.GetStateBackBuffer(), nullptr);
    EXPECT_EQ(broker.AcquireStateFrontBuffer(), nullptr);
}