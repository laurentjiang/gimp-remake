/**
 * @file test_event_bus.cpp
 * @brief Unit tests for EventBus pub/sub system.
 * @author Laurent Jiang
 * @date 2026-01-28
 */

#include "core/event_bus.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

struct TestEvent {
    int value;
};

struct OtherEvent {
    std::string message;
};

}  // namespace

TEST_CASE("EventBus subscribe and publish", "[event_bus][unit]")
{
    gimp::EventBus::instance().clear();

    int receivedValue = 0;
    auto subId = gimp::EventBus::instance().subscribe<TestEvent>(
        [&receivedValue](const TestEvent& event) { receivedValue = event.value; });

    gimp::EventBus::instance().publish(TestEvent{42});

    REQUIRE(receivedValue == 42);
    gimp::EventBus::instance().unsubscribe(subId);
}

TEST_CASE("EventBus unsubscribe stops delivery", "[event_bus][unit]")
{
    gimp::EventBus::instance().clear();

    int callCount = 0;
    auto subId = gimp::EventBus::instance().subscribe<TestEvent>(
        [&callCount](const TestEvent& /*event*/) { ++callCount; });

    gimp::EventBus::instance().publish(TestEvent{1});
    REQUIRE(callCount == 1);

    gimp::EventBus::instance().unsubscribe(subId);
    gimp::EventBus::instance().publish(TestEvent{2});
    REQUIRE(callCount == 1);
}

TEST_CASE("EventBus multiple subscribers receive events", "[event_bus][unit]")
{
    gimp::EventBus::instance().clear();

    int value1 = 0;
    int value2 = 0;

    auto sub1 = gimp::EventBus::instance().subscribe<TestEvent>(
        [&value1](const TestEvent& event) { value1 = event.value; });
    auto sub2 = gimp::EventBus::instance().subscribe<TestEvent>(
        [&value2](const TestEvent& event) { value2 = event.value * 2; });

    gimp::EventBus::instance().publish(TestEvent{10});

    REQUIRE(value1 == 10);
    REQUIRE(value2 == 20);

    gimp::EventBus::instance().unsubscribe(sub1);
    gimp::EventBus::instance().unsubscribe(sub2);
}

TEST_CASE("EventBus type isolation", "[event_bus][unit]")
{
    gimp::EventBus::instance().clear();

    int testValue = 0;
    std::string otherMessage;

    auto sub1 = gimp::EventBus::instance().subscribe<TestEvent>(
        [&testValue](const TestEvent& event) { testValue = event.value; });
    auto sub2 = gimp::EventBus::instance().subscribe<OtherEvent>(
        [&otherMessage](const OtherEvent& event) { otherMessage = event.message; });

    gimp::EventBus::instance().publish(TestEvent{99});

    REQUIRE(testValue == 99);
    REQUIRE(otherMessage.empty());

    gimp::EventBus::instance().publish(OtherEvent{"hello"});

    REQUIRE(testValue == 99);
    REQUIRE(otherMessage == "hello");

    gimp::EventBus::instance().unsubscribe(sub1);
    gimp::EventBus::instance().unsubscribe(sub2);
}

TEST_CASE("EventBus clear removes all subscribers", "[event_bus][unit]")
{
    gimp::EventBus::instance().clear();

    int callCount = 0;
    gimp::EventBus::instance().subscribe<TestEvent>(
        [&callCount](const TestEvent& /*event*/) { ++callCount; });

    gimp::EventBus::instance().publish(TestEvent{1});
    REQUIRE(callCount == 1);

    gimp::EventBus::instance().clear();
    gimp::EventBus::instance().publish(TestEvent{2});
    REQUIRE(callCount == 1);
}

TEST_CASE("EventBus subscription IDs are unique", "[event_bus][unit]")
{
    gimp::EventBus::instance().clear();

    auto id1 = gimp::EventBus::instance().subscribe<TestEvent>([](const TestEvent&) {});
    auto id2 = gimp::EventBus::instance().subscribe<TestEvent>([](const TestEvent&) {});
    auto id3 = gimp::EventBus::instance().subscribe<OtherEvent>([](const OtherEvent&) {});

    REQUIRE(id1 != id2);
    REQUIRE(id2 != id3);
    REQUIRE(id1 != id3);

    gimp::EventBus::instance().clear();
}

TEST_CASE("EventBus publish with no subscribers does not crash", "[event_bus][unit]")
{
    gimp::EventBus::instance().clear();

    REQUIRE_NOTHROW(gimp::EventBus::instance().publish(TestEvent{100}));
}

TEST_CASE("EventBus unsubscribe with invalid ID does not crash", "[event_bus][unit]")
{
    gimp::EventBus::instance().clear();

    REQUIRE_NOTHROW(gimp::EventBus::instance().unsubscribe(999999));
}

