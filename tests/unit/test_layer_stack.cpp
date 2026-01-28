/**
 * @file test_layer_stack.cpp
 * @brief Unit tests for LayerStack.
 * @author Laurent Jiang
 * @date 2026-01-28
 */

#include "core/layer.h"
#include "core/layer_stack.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("LayerStack initializes empty", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    REQUIRE(stack.empty());
    REQUIRE(stack.count() == 0);
}

TEST_CASE("LayerStack addLayer increases count", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    auto layer1 = std::make_shared<gimp::Layer>(100, 100);
    stack.addLayer(layer1);

    REQUIRE(stack.count() == 1);
    REQUIRE_FALSE(stack.empty());
    REQUIRE(stack[0] == layer1);
}

TEST_CASE("LayerStack addLayer ignores nullptr", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    stack.addLayer(nullptr);

    REQUIRE(stack.empty());
    REQUIRE(stack.count() == 0);
}

TEST_CASE("LayerStack removeLayer decreases count", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    auto layer1 = std::make_shared<gimp::Layer>(100, 100);
    auto layer2 = std::make_shared<gimp::Layer>(100, 100);
    stack.addLayer(layer1);
    stack.addLayer(layer2);

    REQUIRE(stack.count() == 2);

    stack.removeLayer(layer1);

    REQUIRE(stack.count() == 1);
    REQUIRE(stack[0] == layer2);
}

TEST_CASE("LayerStack removeLayer with non-existent layer is safe", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    auto layer1 = std::make_shared<gimp::Layer>(100, 100);
    auto layer2 = std::make_shared<gimp::Layer>(100, 100);
    stack.addLayer(layer1);

    REQUIRE_NOTHROW(stack.removeLayer(layer2));
    REQUIRE(stack.count() == 1);
}

TEST_CASE("LayerStack insertLayer at index", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    auto layer1 = std::make_shared<gimp::Layer>(100, 100);
    layer1->setName("Layer1");
    auto layer2 = std::make_shared<gimp::Layer>(100, 100);
    layer2->setName("Layer2");
    auto layer3 = std::make_shared<gimp::Layer>(100, 100);
    layer3->setName("Layer3");

    stack.addLayer(layer1);
    stack.addLayer(layer3);
    stack.insertLayer(1, layer2);

    REQUIRE(stack.count() == 3);
    REQUIRE(stack[0]->name() == "Layer1");
    REQUIRE(stack[1]->name() == "Layer2");
    REQUIRE(stack[2]->name() == "Layer3");
}

TEST_CASE("LayerStack insertLayer at out-of-bounds index appends", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    auto layer1 = std::make_shared<gimp::Layer>(100, 100);
    layer1->setName("First");
    auto layer2 = std::make_shared<gimp::Layer>(100, 100);
    layer2->setName("Second");

    stack.addLayer(layer1);
    stack.insertLayer(100, layer2);

    REQUIRE(stack.count() == 2);
    REQUIRE(stack[1]->name() == "Second");
}

TEST_CASE("LayerStack insertLayer ignores nullptr", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    stack.insertLayer(0, nullptr);

    REQUIRE(stack.empty());
}

TEST_CASE("LayerStack moveLayer reorders correctly", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    auto layer1 = std::make_shared<gimp::Layer>(100, 100);
    layer1->setName("A");
    auto layer2 = std::make_shared<gimp::Layer>(100, 100);
    layer2->setName("B");
    auto layer3 = std::make_shared<gimp::Layer>(100, 100);
    layer3->setName("C");

    stack.addLayer(layer1);
    stack.addLayer(layer2);
    stack.addLayer(layer3);

    REQUIRE(stack.moveLayer(0, 2));

    REQUIRE(stack[0]->name() == "B");
    REQUIRE(stack[1]->name() == "C");
    REQUIRE(stack[2]->name() == "A");
}

TEST_CASE("LayerStack moveLayer same index returns true", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    auto layer = std::make_shared<gimp::Layer>(100, 100);
    stack.addLayer(layer);

    REQUIRE(stack.moveLayer(0, 0));
    REQUIRE(stack.count() == 1);
}

TEST_CASE("LayerStack moveLayer invalid fromIndex returns false", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    auto layer = std::make_shared<gimp::Layer>(100, 100);
    stack.addLayer(layer);

    REQUIRE_FALSE(stack.moveLayer(5, 0));
}

TEST_CASE("LayerStack moveLayer clamps toIndex", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    auto layer1 = std::make_shared<gimp::Layer>(100, 100);
    layer1->setName("First");
    auto layer2 = std::make_shared<gimp::Layer>(100, 100);
    layer2->setName("Second");

    stack.addLayer(layer1);
    stack.addLayer(layer2);

    REQUIRE(stack.moveLayer(0, 100));
    REQUIRE(stack[1]->name() == "First");
}

TEST_CASE("LayerStack iteration", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    auto layer1 = std::make_shared<gimp::Layer>(100, 100);
    auto layer2 = std::make_shared<gimp::Layer>(100, 100);
    auto layer3 = std::make_shared<gimp::Layer>(100, 100);

    stack.addLayer(layer1);
    stack.addLayer(layer2);
    stack.addLayer(layer3);

    int count = 0;
    for (const auto& layer : stack) {
        REQUIRE(layer != nullptr);
        ++count;
    }

    REQUIRE(count == 3);
}

TEST_CASE("LayerStack reverse iteration", "[layer_stack][unit]")
{
    gimp::LayerStack stack;

    auto layer1 = std::make_shared<gimp::Layer>(100, 100);
    layer1->setName("First");
    auto layer2 = std::make_shared<gimp::Layer>(100, 100);
    layer2->setName("Second");
    auto layer3 = std::make_shared<gimp::Layer>(100, 100);
    layer3->setName("Third");

    stack.addLayer(layer1);
    stack.addLayer(layer2);
    stack.addLayer(layer3);

    std::vector<std::string> names;
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        names.push_back((*it)->name());
    }

    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "Third");
    REQUIRE(names[1] == "Second");
    REQUIRE(names[2] == "First");
}

