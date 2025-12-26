#pragma once

#include <SFML/Graphics.hpp>
#include "Item.hpp"
#include <functional>

class InventoryWindow;

struct DragSource
{
    InventoryWindow* inventory = nullptr;
    int slotIndex = -1;
};

class DragDropManager : public sf::Drawable
{
public:
    using DropCallback = std::function<void(InventoryWindow* sourceInv, int sourceSlot,
                                             InventoryWindow* targetInv, int targetSlot)>;
    using ClearHighlightsCallback = std::function<void()>;

    DragDropManager() = default;

    void setDropCallback(DropCallback callback)
    {
        m_dropCallback = std::move(callback);
    }

    void setClearHighlightsCallback(ClearHighlightsCallback callback)
    {
        m_clearHighlightsCallback = std::move(callback);
    }

    bool isDragging() const { return m_isDragging; }

    void startDrag(const Item& item, InventoryWindow* sourceInventory, int slotIndex, const sf::Vector2f& mousePos)
    {
        m_isDragging = true;
        m_draggedItem = item;
        m_source.inventory = sourceInventory;
        m_source.slotIndex = slotIndex;
        m_mousePos = mousePos;

        // 고스트 이미지 설정
        m_ghostRect.setSize({46.f, 46.f});
        m_ghostRect.setFillColor(sf::Color(item.color.r, item.color.g, item.color.b, 180));
        m_ghostRect.setOutlineThickness(2.f);
        m_ghostRect.setOutlineColor(sf::Color::White);
        updateGhostPosition();
    }

    void updateMousePosition(const sf::Vector2f& mousePos)
    {
        m_mousePos = mousePos;
        updateGhostPosition();
    }

    void endDrag(InventoryWindow* targetInventory, int targetSlot)
    {
        if (!m_isDragging) return;

        if (m_dropCallback)
        {
            m_dropCallback(m_source.inventory, m_source.slotIndex, targetInventory, targetSlot);
        }

        resetDragState();
    }

    void cancelDrag()
    {
        if (!m_isDragging) return;

        // 취소 시 원래 위치로 복구 (targetSlot = -1로 콜백 호출)
        if (m_dropCallback)
        {
            m_dropCallback(m_source.inventory, m_source.slotIndex, nullptr, -1);
        }

        resetDragState();
    }

private:
    void resetDragState()
    {
        // 모든 인벤토리의 하이라이트 클리어 (상태 리셋 전에 호출)
        if (m_clearHighlightsCallback)
        {
            m_clearHighlightsCallback();
        }

        m_isDragging = false;
        m_source.inventory = nullptr;
        m_source.slotIndex = -1;
    }

public:

    const Item& getDraggedItem() const { return m_draggedItem; }
    DragSource getSource() const { return m_source; }

    bool handleEvent(const sf::Event& event)
    {
        if (!m_isDragging) return false;

        if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
        {
            updateMousePosition({static_cast<float>(mouseMoved->position.x),
                                  static_cast<float>(mouseMoved->position.y)});
            return true;
        }
        else if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
        {
            if (keyPressed->code == sf::Keyboard::Key::Escape)
            {
                cancelDrag();
                return true;
            }
        }

        return false;
    }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        if (m_isDragging)
        {
            target.draw(m_ghostRect, states);
        }
    }

    void updateGhostPosition()
    {
        m_ghostRect.setPosition({m_mousePos.x - 23.f, m_mousePos.y - 23.f});
    }

    bool m_isDragging = false;
    Item m_draggedItem;
    DragSource m_source;
    sf::Vector2f m_mousePos;
    sf::RectangleShape m_ghostRect;
    DropCallback m_dropCallback;
    ClearHighlightsCallback m_clearHighlightsCallback;
};
