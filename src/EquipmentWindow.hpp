#pragma once

#include "Window.hpp"
#include "InventorySlot.hpp"
#include "DragDropManager.hpp"
#include <vector>
#include <array>
#include <memory>

class EquipmentWindow : public sf::Drawable
{
public:
    static constexpr float SLOT_SIZE = 50.f;
    static constexpr float AVATAR_SIZE = 100.f;

    EquipmentWindow(const sf::Vector2f& position, const sf::Font& font)
        : m_font(font)
        , m_window(position, calculateWindowSize(), font, "Equipment")
    {
        m_window.setOnClose([this]() {
            m_window.setVisible(false);
        });

        // 아바타 플레이스홀더 (중앙)
        m_avatarRect.setSize({AVATAR_SIZE, AVATAR_SIZE});
        m_avatarRect.setFillColor(sf::Color{80, 80, 80});
        m_avatarRect.setOutlineThickness(2.f);
        m_avatarRect.setOutlineColor(sf::Color{100, 100, 100});

        createSlots();
        updatePositions();
    }

    void setDragDropManager(DragDropManager* manager)
    {
        m_dragDropManager = manager;
    }

    void setRenderWindow(sf::RenderWindow* window)
    {
        m_window.setRenderWindow(window);
    }

    void setUIView(const sf::View* view)
    {
        m_window.setUIView(view);
    }

    void setItemsTexture(const sf::Texture* texture)
    {
        m_itemsTexture = texture;
        for (auto& slot : m_slots)
        {
            slot->setItemsTexture(texture);
        }
    }

    void setWeaponsTexture(const sf::Texture* texture)
    {
        m_weaponsTexture = texture;
        for (auto& slot : m_slots)
        {
            slot->setWeaponsTexture(texture);
        }
    }

    void setItem(EquipmentSlot slot, const OptionalItem& item)
    {
        int index = static_cast<int>(slot) - 1;  // None = 0이므로 -1
        if (index >= 0 && index < static_cast<int>(m_slots.size()))
        {
            m_items[index] = item;
            m_slots[index]->setItem(item);
        }
    }

    OptionalItem getItem(EquipmentSlot slot) const
    {
        int index = static_cast<int>(slot) - 1;
        if (index >= 0 && index < static_cast<int>(m_items.size()))
        {
            return m_items[index];
        }
        return std::nullopt;
    }

    OptionalItem getItem(int slotIndex) const
    {
        if (slotIndex >= 0 && slotIndex < static_cast<int>(m_items.size()))
        {
            return m_items[slotIndex];
        }
        return std::nullopt;
    }

    void setItemByIndex(int index, const OptionalItem& item)
    {
        if (index >= 0 && index < static_cast<int>(m_slots.size()))
        {
            m_items[index] = item;
            m_slots[index]->setItem(item);
        }
    }

    bool canEquipItem(const Item& item, int slotIndex) const
    {
        // 슬롯 인덱스에 맞는 장비 타입인지 확인
        EquipmentSlot requiredSlot = static_cast<EquipmentSlot>(slotIndex + 1);
        return item.equipSlot == requiredSlot;
    }

    EquipmentSlot getSlotType(int slotIndex) const
    {
        return static_cast<EquipmentSlot>(slotIndex + 1);
    }

    bool handleEvent(const sf::Event& event)
    {
        if (!m_window.isVisible()) return false;

        // 드래그 앤 드롭 처리 (클릭-토글 방식)
        if (m_dragDropManager)
        {
            // 클릭 처리 - 드래그 시작 또는 드롭
            if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
            {
                if (mousePressed->button == sf::Mouse::Button::Left)
                {
                    // 픽셀 좌표를 UI 뷰 좌표로 변환
                    sf::Vector2f mousePos = m_dragDropManager->mapPixelToUI(mousePressed->position);

                    // 드래그 중이면 드롭 처리
                    if (m_dragDropManager->isDragging())
                    {
                        if (m_window.contains(mousePos))
                        {
                            int targetSlot = getSlotAtPosition(mousePos);

                            // 장비 슬롯에 맞는 아이템인지 확인
                            if (targetSlot >= 0)
                            {
                                const Item& draggedItem = m_dragDropManager->getDraggedItem();
                                if (!canEquipItem(draggedItem, targetSlot))
                                {
                                    // 장비 불가 - 취소 처리
                                    m_dragDropManager->cancelDrag();
                                    return true;
                                }
                            }

                            m_dragDropManager->endDragToEquipment(this, targetSlot);
                            return true;
                        }
                        // 이 창 밖이면 다른 창이 처리하도록 패스
                        return false;
                    }

                    // 드래그 시작
                    int slotIndex = getSlotAtPosition(mousePos);
                    if (slotIndex >= 0 && m_slots[slotIndex]->hasItem())
                    {
                        OptionalItem item = m_items[slotIndex];
                        if (item)
                        {
                            m_dragDropManager->startDragFromEquipment(*item, this, slotIndex, mousePos);
                            m_items[slotIndex] = std::nullopt;
                            m_slots[slotIndex]->setItem(std::nullopt);
                        }
                        return true;
                    }
                }
            }

            // 마우스 이동 중 호버 슬롯 하이라이트
            if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
            {
                sf::Vector2f mousePos = m_dragDropManager->mapPixelToUI(mouseMoved->position);

                if (m_dragDropManager->isDragging())
                {
                    int hoverSlot = getSlotAtPosition(mousePos);
                    for (size_t i = 0; i < m_slots.size(); ++i)
                    {
                        bool canEquip = false;
                        if (static_cast<int>(i) == hoverSlot)
                        {
                            const Item& draggedItem = m_dragDropManager->getDraggedItem();
                            canEquip = canEquipItem(draggedItem, hoverSlot);
                        }
                        m_slots[i]->setHighlight(static_cast<int>(i) == hoverSlot && canEquip);
                    }
                }
            }
        }

        // 윈도우 이벤트 처리
        if (m_window.handleEvent(event))
        {
            updatePositions();
            return true;
        }

        return false;
    }

    int getSlotAtPosition(const sf::Vector2f& pos) const
    {
        for (size_t i = 0; i < m_slots.size(); ++i)
        {
            if (m_slots[i]->contains(pos))
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    bool isVisible() const { return m_window.isVisible(); }
    void setVisible(bool visible) { m_window.setVisible(visible); }

    void setPosition(const sf::Vector2f& position)
    {
        m_window.setPosition(position);
        updatePositions();
    }

    void clearAllHighlights()
    {
        for (auto& slot : m_slots)
        {
            slot->setHighlight(false);
            slot->clearHover();
        }
    }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        if (!m_window.isVisible()) return;

        target.draw(m_window, states);
        target.draw(m_avatarRect, states);

        for (const auto& slot : m_slots)
        {
            target.draw(*slot, states);
        }

        // 슬롯 라벨
        for (const auto& label : m_slotLabels)
        {
            target.draw(label, states);
        }
    }

    static sf::Vector2f calculateWindowSize()
    {
        // 3열 레이아웃: 왼쪽 슬롯 | 아바타 | 오른쪽 슬롯
        float width = SLOT_SIZE * 3 + 40.f + AVATAR_SIZE;
        float height = SLOT_SIZE * 3 + 60.f + 25.f;
        return {width, height};
    }

    void createSlots()
    {
        // 6개 장비 슬롯: Weapon, Shield, Helmet, Armor, Gloves, Boots
        for (int i = 0; i < 6; ++i)
        {
            auto slot = std::make_shared<InventorySlot>(
                sf::Vector2f{0.f, 0.f},
                sf::Vector2f{SLOT_SIZE, SLOT_SIZE}
            );
            m_slots.push_back(slot);
            m_items.push_back(std::nullopt);

            // 라벨 생성
            m_slotLabels.emplace_back(m_font, getSlotName(static_cast<EquipmentSlot>(i + 1)), 10);
            m_slotLabels.back().setFillColor(sf::Color{180, 180, 180});
        }
    }

    std::string getSlotName(EquipmentSlot slot) const
    {
        switch (slot)
        {
            case EquipmentSlot::Weapon: return "Weapon";
            case EquipmentSlot::Shield: return "Shield";
            case EquipmentSlot::Helmet: return "Helmet";
            case EquipmentSlot::Armor: return "Armor";
            case EquipmentSlot::Gloves: return "Gloves";
            case EquipmentSlot::Boots: return "Boots";
            default: return "";
        }
    }

    void updatePositions()
    {
        sf::Vector2f contentPos = m_window.getContentPosition();
        float padding = 10.f;

        // 아바타 중앙 배치
        float avatarX = contentPos.x + (calculateWindowSize().x - AVATAR_SIZE) / 2.f;
        float avatarY = contentPos.y + padding + SLOT_SIZE + 10.f;
        m_avatarRect.setPosition({avatarX, avatarY});

        // 슬롯 배치
        // 왼쪽 열: Helmet (위), Weapon (중간), Gloves (아래)
        // 오른쪽 열: Armor (위), Shield (중간), Boots (아래)

        float leftX = contentPos.x + padding;
        float rightX = contentPos.x + calculateWindowSize().x - padding - SLOT_SIZE;
        float topY = contentPos.y + padding;
        float midY = topY + SLOT_SIZE + 10.f;
        float botY = midY + SLOT_SIZE + 10.f;

        // Weapon (0) - 왼쪽 중간
        m_slots[0]->setPosition({leftX, midY});
        // Shield (1) - 오른쪽 중간
        m_slots[1]->setPosition({rightX, midY});
        // Helmet (2) - 왼쪽 위
        m_slots[2]->setPosition({leftX, topY});
        // Armor (3) - 오른쪽 위
        m_slots[3]->setPosition({rightX, topY});
        // Gloves (4) - 왼쪽 아래
        m_slots[4]->setPosition({leftX, botY});
        // Boots (5) - 오른쪽 아래
        m_slots[5]->setPosition({rightX, botY});

        // 라벨 위치 업데이트
        for (size_t i = 0; i < m_slots.size(); ++i)
        {
            sf::Vector2f slotPos = m_slots[i]->getPosition();
            m_slotLabels[i].setPosition({slotPos.x, slotPos.y + SLOT_SIZE + 2.f});
        }
    }

    const sf::Font& m_font;
    Window m_window;
    std::vector<std::shared_ptr<InventorySlot>> m_slots;
    std::vector<OptionalItem> m_items;
    std::vector<sf::Text> m_slotLabels;
    DragDropManager* m_dragDropManager = nullptr;

    const sf::Texture* m_itemsTexture = nullptr;
    const sf::Texture* m_weaponsTexture = nullptr;

    sf::RectangleShape m_avatarRect;
};
