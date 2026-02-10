/**
 * @file test_transform_state.cpp
 * @brief Unit tests for TransformState class.
 * @author Laurent Jiang
 * @date 2026-02-10
 */

#include "core/transform_state.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace gimp;
using Catch::Matchers::WithinAbs;

TEST_CASE("TransformState default construction", "[transform_state]")
{
    TransformState state;

    SECTION("is identity by default")
    {
        REQUIRE(state.isIdentity());
    }

    SECTION("has zero translation")
    {
        REQUIRE_THAT(state.translation().x(), WithinAbs(0.0, 0.001));
        REQUIRE_THAT(state.translation().y(), WithinAbs(0.0, 0.001));
    }

    SECTION("has unit scale")
    {
        REQUIRE_THAT(state.scale().width(), WithinAbs(1.0, 0.001));
        REQUIRE_THAT(state.scale().height(), WithinAbs(1.0, 0.001));
    }

    SECTION("has zero rotation")
    {
        REQUIRE_THAT(state.rotation(), WithinAbs(0.0, 0.001));
    }
}

TEST_CASE("TransformState with bounds", "[transform_state]")
{
    QRectF bounds(100.0, 50.0, 200.0, 150.0);
    TransformState state(bounds);

    SECTION("stores original bounds")
    {
        REQUIRE(state.originalBounds() == bounds);
    }

    SECTION("transformed bounds equals original when identity")
    {
        QRectF transformed = state.transformedBounds();
        REQUIRE_THAT(transformed.left(), WithinAbs(100.0, 0.001));
        REQUIRE_THAT(transformed.top(), WithinAbs(50.0, 0.001));
        REQUIRE_THAT(transformed.width(), WithinAbs(200.0, 0.001));
        REQUIRE_THAT(transformed.height(), WithinAbs(150.0, 0.001));
    }
}

TEST_CASE("TransformState translation", "[transform_state]")
{
    QRectF bounds(0.0, 0.0, 100.0, 100.0);
    TransformState state(bounds);

    SECTION("setTranslation sets translation")
    {
        state.setTranslation(QPointF(50.0, 30.0));

        REQUIRE_THAT(state.translation().x(), WithinAbs(50.0, 0.001));
        REQUIRE_THAT(state.translation().y(), WithinAbs(30.0, 0.001));
        REQUIRE_FALSE(state.isIdentity());
    }

    SECTION("translate accumulates")
    {
        state.translate(QPointF(10.0, 5.0));
        state.translate(QPointF(20.0, 15.0));

        REQUIRE_THAT(state.translation().x(), WithinAbs(30.0, 0.001));
        REQUIRE_THAT(state.translation().y(), WithinAbs(20.0, 0.001));
    }

    SECTION("translation affects transformed bounds")
    {
        state.setTranslation(QPointF(50.0, 30.0));
        QRectF transformed = state.transformedBounds();

        REQUIRE_THAT(transformed.left(), WithinAbs(50.0, 0.001));
        REQUIRE_THAT(transformed.top(), WithinAbs(30.0, 0.001));
    }
}

TEST_CASE("TransformState scaling", "[transform_state]")
{
    QRectF bounds(0.0, 0.0, 100.0, 100.0);
    TransformState state(bounds);

    SECTION("setScale sets scale factors")
    {
        state.setScale(QSizeF(2.0, 0.5));

        REQUIRE_THAT(state.scale().width(), WithinAbs(2.0, 0.001));
        REQUIRE_THAT(state.scale().height(), WithinAbs(0.5, 0.001));
        REQUIRE_FALSE(state.isIdentity());
    }

    SECTION("scale affects transformed bounds size")
    {
        state.setScale(QSizeF(2.0, 1.5));
        QRectF transformed = state.transformedBounds();

        REQUIRE_THAT(transformed.width(), WithinAbs(200.0, 0.001));
        REQUIRE_THAT(transformed.height(), WithinAbs(150.0, 0.001));
    }
}

TEST_CASE("TransformState reset", "[transform_state]")
{
    QRectF bounds(10.0, 20.0, 100.0, 80.0);
    TransformState state(bounds);

    state.setTranslation(QPointF(50.0, 30.0));
    state.setScale(QSizeF(2.0, 1.5));
    state.setRotation(45.0);

    SECTION("reset restores identity")
    {
        REQUIRE_FALSE(state.isIdentity());

        state.reset();

        REQUIRE(state.isIdentity());
        REQUIRE_THAT(state.translation().x(), WithinAbs(0.0, 0.001));
        REQUIRE_THAT(state.scale().width(), WithinAbs(1.0, 0.001));
        REQUIRE_THAT(state.rotation(), WithinAbs(0.0, 0.001));
    }

    SECTION("reset preserves original bounds")
    {
        state.reset();

        REQUIRE(state.originalBounds() == bounds);
    }
}

TEST_CASE("TransformState handle rects", "[transform_state]")
{
    QRectF bounds(100.0, 100.0, 200.0, 150.0);
    TransformState state(bounds);
    qreal handleSize = 10.0;

    SECTION("returns 8 handles")
    {
        auto handles = state.getHandleRects(handleSize);
        REQUIRE(handles.size() == 8);
    }

    SECTION("handles are centered on corners/edges")
    {
        auto handles = state.getHandleRects(handleSize);

        // TopLeft handle should be centered on (100, 100)
        REQUIRE_THAT(handles[0].center().x(), WithinAbs(100.0, 0.001));
        REQUIRE_THAT(handles[0].center().y(), WithinAbs(100.0, 0.001));

        // BottomRight handle should be centered on (300, 250)
        REQUIRE_THAT(handles[4].center().x(), WithinAbs(300.0, 0.001));
        REQUIRE_THAT(handles[4].center().y(), WithinAbs(250.0, 0.001));
    }
}

TEST_CASE("TransformState hitTestHandle", "[transform_state]")
{
    QRectF bounds(100.0, 100.0, 200.0, 150.0);
    TransformState state(bounds);
    qreal handleSize = 10.0;

    SECTION("returns correct handle for corner hit")
    {
        auto handle = state.hitTestHandle(QPointF(100.0, 100.0), handleSize);
        REQUIRE(handle == TransformHandle::TopLeft);
    }

    SECTION("returns None for miss")
    {
        auto handle = state.hitTestHandle(QPointF(200.0, 175.0), handleSize);
        REQUIRE(handle == TransformHandle::None);
    }
}

TEST_CASE("TransformState getAnchorForHandle", "[transform_state]")
{
    QRectF bounds(100.0, 100.0, 200.0, 150.0);
    TransformState state(bounds);

    SECTION("TopLeft anchor is BottomRight")
    {
        QPointF anchor = state.getAnchorForHandle(TransformHandle::TopLeft);
        REQUIRE_THAT(anchor.x(), WithinAbs(300.0, 0.001));
        REQUIRE_THAT(anchor.y(), WithinAbs(250.0, 0.001));
    }

    SECTION("BottomRight anchor is TopLeft")
    {
        QPointF anchor = state.getAnchorForHandle(TransformHandle::BottomRight);
        REQUIRE_THAT(anchor.x(), WithinAbs(100.0, 0.001));
        REQUIRE_THAT(anchor.y(), WithinAbs(100.0, 0.001));
    }
}
