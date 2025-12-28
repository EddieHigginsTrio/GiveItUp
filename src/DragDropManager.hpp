#pragma once

#include <SFML/Graphics.hpp>
#include "Item.hpp"
#include <functional>

class InventoryWindow;
class EquipmentWindow;

enum class DragSourceType
{
    None,
    Inventory,
    Equipment
};

struct DragSource
{
    DragSourceType type = DragSourceType::None;
    InventoryWindow* inventory = nullptr;
    EquipmentWindow* equipment = nullptr;
    int slotIndex = -1;
};

class DragDropManager : public sf::Drawable
{
public:
    using DropCallback = std::function<void(InventoryWindow* sourceInv, int sourceSlot,
                                             InventoryWindow* targetInv, int targetSlot)>;
    using EquipmentDropCallback = std::function<void(const DragSource& source,
                                                      EquipmentWindow* targetEquip, int targetSlot)>;
    using ClearHighlightsCallback = std::function<void()>;

    DragDropManager() {}

    void setRenderWindow(sf::RenderWindow* window) { m_renderWindow = window; }
    void setUIView(const sf::View* view) { m_uiView = view; }

    // 픽셀 좌표를 UI 뷰 좌표로 변환
    sf::Vector2f mapPixelToUI(const sf::Vector2i& pixel) const
    {
        if (m_renderWindow && m_uiView)
        {
            return m_renderWindow->mapPixelToCoords(pixel, *m_uiView);
        }
        return sf::Vector2f(static_cast<float>(pixel.x), static_cast<float>(pixel.y));
    }

    void setDropCallback(DropCallback callback)
    {
        m_dropCallback = std::move(callback);
    }

    void setClearHighlightsCallback(ClearHighlightsCallback callback)
    {
        m_clearHighlightsCallback = std::move(callback);
    }

    void setEquipmentDropCallback(EquipmentDropCallback callback)
    {
        m_equipmentDropCallback = std::move(callback);
    }

    void setItemsTexture(const sf::Texture* texture) { m_itemsTexture = texture; }
    void setWeaponsTexture(const sf::Texture* texture) { m_weaponsTexture = texture; }

    bool isDragging() const { return m_isDragging; }

    void startDrag(const Item& item, InventoryWindow* sourceInventory, int slotIndex, const sf::Vector2f& mousePos)
    {
        m_isDragging = true;
        m_draggedItem = item;
        m_source.type = DragSourceType::Inventory;
        m_source.inventory = sourceInventory;
        m_source.equipment = nullptr;
        m_source.slotIndex = slotIndex;
        m_mousePos = mousePos;

        setupGhostRect(item);
    }

    void startDragFromEquipment(const Item& item, EquipmentWindow* sourceEquipment, int slotIndex, const sf::Vector2f& mousePos)
    {
        m_isDragging = true;
        m_draggedItem = item;
        m_source.type = DragSourceType::Equipment;
        m_source.inventory = nullptr;
        m_source.equipment = sourceEquipment;
        m_source.slotIndex = slotIndex;
        m_mousePos = mousePos;

        setupGhostRect(item);
    }

    void setupGhostRect(const Item& item)
    {
        m_useSprite = false;
        m_ghostSprite = std::nullopt;

        if (item.hasSprite())
        {
            const sf::Texture* texture = nullptr;
            if (item.sheetType == SpriteSheetType::Items)
                texture = m_itemsTexture;
            else if (item.sheetType == SpriteSheetType::Weapons)
                texture = m_weaponsTexture;

            if (texture)
            {
                m_useSprite = true;
                m_ghostSprite = sf::Sprite(*texture);
                m_ghostSprite->setTextureRect(sf::IntRect(
                    {item.spriteX * SPRITE_WIDTH, item.spriteY * SPRITE_HEIGHT},
                    {SPRITE_WIDTH, SPRITE_HEIGHT}
                ));
                m_ghostSprite->setColor(sf::Color(255, 255, 255, 200));
                float scaleX = 46.f / SPRITE_WIDTH;
                float scaleY = 46.f / SPRITE_HEIGHT;
                float scale = std::min(scaleX, scaleY);
                m_ghostSprite->setScale({scale, scale});
            }
        }

        if (!m_useSprite)
        {
            m_ghostRect.setSize({46.f, 46.f});
            m_ghostRect.setFillColor(sf::Color(item.color.r, item.color.g, item.color.b, 180));
            m_ghostRect.setOutlineThickness(2.f);
            m_ghostRect.setOutlineColor(sf::Color::White);
        }

        updateGhostPosition();
    }

    static constexpr int SPRITE_WIDTH = 352;   // 스프라이트 타일 너비 (2816/8)
    static constexpr int SPRITE_HEIGHT = 384;  // 스프라이트 타일 높이 (1536/4)

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

    void endDragToEquipment(EquipmentWindow* targetEquipment, int targetSlot)
    {
        if (!m_isDragging) return;

        if (m_equipmentDropCallback)
        {
            m_equipmentDropCallback(m_source, targetEquipment, targetSlot);
        }

        resetDragState();
    }

    void cancelDrag()
    {
        if (!m_isDragging) return;

        // 취소 시 원래 위치로 복구
        if (m_source.type == DragSourceType::Inventory && m_dropCallback)
        {
            m_dropCallback(m_source.inventory, m_source.slotIndex, nullptr, -1);
        }
        else if (m_source.type == DragSourceType::Equipment && m_equipmentDropCallback)
        {
            m_equipmentDropCallback(m_source, nullptr, -1);
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
        m_source.type = DragSourceType::None;
        m_source.inventory = nullptr;
        m_source.equipment = nullptr;
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
            sf::Vector2f uiPos = mapPixelToUI(mouseMoved->position);
            updateMousePosition(uiPos);
            // false를 반환해서 인벤토리에서도 하이라이트 처리 가능하게 함
            return false;
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
            if (m_useSprite && m_ghostSprite.has_value())
            {
                target.draw(*m_ghostSprite, states);
            }
            else
            {
                target.draw(m_ghostRect, states);
            }
        }
    }

    void updateGhostPosition()
    {
        m_ghostRect.setPosition({m_mousePos.x - 23.f, m_mousePos.y - 23.f});
        if (m_ghostSprite.has_value())
        {
            m_ghostSprite->setPosition({m_mousePos.x - 23.f, m_mousePos.y - 23.f});
        }
    }

    bool m_isDragging = false;
    bool m_useSprite = false;
    Item m_draggedItem;
    DragSource m_source;
    sf::Vector2f m_mousePos;
    sf::RectangleShape m_ghostRect;
    std::optional<sf::Sprite> m_ghostSprite;
    DropCallback m_dropCallback;
    EquipmentDropCallback m_equipmentDropCallback;
    ClearHighlightsCallback m_clearHighlightsCallback;

    const sf::Texture* m_itemsTexture = nullptr;
    const sf::Texture* m_weaponsTexture = nullptr;

    sf::RenderWindow* m_renderWindow = nullptr;
    const sf::View* m_uiView = nullptr;
};
