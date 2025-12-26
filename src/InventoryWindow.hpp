#pragma once

#include "Window.hpp"
#include "InventorySlot.hpp"
#include "DragDropManager.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

class InventoryWindow : public sf::Drawable
{
public:
    static constexpr int GRID_COLS = 5;
    static constexpr int GRID_ROWS = 10;
    static constexpr float SLOT_SIZE = 50.f;
    static constexpr float SLOT_PADDING = 5.f;
    static constexpr float DRAG_THRESHOLD = 5.f;

    InventoryWindow(const sf::Vector2f& position, const sf::Font& font, const std::string& title = "Inventory")
        : m_font(font)
        , m_window(position, calculateWindowSize(), font, title)
    {
        m_window.setOnClose([this]() {
            m_window.setVisible(false);
        });

        // 스크롤바 설정
        float contentHeight = m_window.getContentSize().y;
        m_scrollbar.setSize({10.f, contentHeight});
        m_scrollbar.setFillColor(sf::Color{60, 60, 60});

        m_scrollThumb.setSize({10.f, 50.f});
        m_scrollThumb.setFillColor(sf::Color{100, 100, 100});

        // 아이템 배열 초기화
        m_items.resize(GRID_COLS * GRID_ROWS);

        updateScrollbarPosition();
        createSlots();
    }

    void setDragDropManager(DragDropManager* manager)
    {
        m_dragDropManager = manager;
    }

    void setItem(int index, const OptionalItem& item)
    {
        if (index >= 0 && index < static_cast<int>(m_items.size()))
        {
            m_items[index] = item;
            m_slots[index]->setItem(item);
        }
    }

    OptionalItem getItem(int index) const
    {
        if (index >= 0 && index < static_cast<int>(m_items.size()))
        {
            return m_items[index];
        }
        return std::nullopt;
    }

    void swapItems(int index1, int index2)
    {
        if (index1 >= 0 && index1 < static_cast<int>(m_items.size()) &&
            index2 >= 0 && index2 < static_cast<int>(m_items.size()))
        {
            std::swap(m_items[index1], m_items[index2]);
            m_slots[index1]->setItem(m_items[index1]);
            m_slots[index2]->setItem(m_items[index2]);
        }
    }

    bool handleEvent(const sf::Event& event)
    {
        if (!m_window.isVisible()) return false;

        sf::Vector2f contentPos = m_window.getContentPosition();
        sf::Vector2f contentSize = m_window.getContentSize();
        sf::FloatRect contentBounds(contentPos, contentSize);

        // 드래그 앤 드롭 처리
        if (m_dragDropManager)
        {
            // 드래그 중 마우스 릴리즈 - 드롭 처리
            if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>())
            {
                if (mouseReleased->button == sf::Mouse::Button::Left)
                {
                    sf::Vector2f mousePos(static_cast<float>(mouseReleased->position.x),
                                           static_cast<float>(mouseReleased->position.y));

                    if (m_dragDropManager->isDragging())
                    {
                        // 이 인벤토리 영역 안에서 드롭했는지 확인
                        if (m_window.contains(mousePos))
                        {
                            int targetSlot = getSlotAtPosition(mousePos);
                            m_dragDropManager->endDrag(this, targetSlot);
                            m_isPotentialDrag = false;
                            return true;
                        }
                        // 이 인벤토리 밖이면 다른 인벤토리가 처리하도록 패스
                        return false;
                    }

                    if (m_isPotentialDrag)
                    {
                        m_isPotentialDrag = false;
                        return true;
                    }
                }
            }

            // 드래그 시작 감지
            if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
            {
                if (mousePressed->button == sf::Mouse::Button::Left)
                {
                    sf::Vector2f mousePos(static_cast<float>(mousePressed->position.x),
                                           static_cast<float>(mousePressed->position.y));

                    int slotIndex = getSlotAtPosition(mousePos);
                    if (slotIndex >= 0 && m_slots[slotIndex]->hasItem())
                    {
                        m_isPotentialDrag = true;
                        m_dragStartPos = mousePos;
                        m_dragStartSlot = slotIndex;
                        return true;
                    }
                }
            }

            // 마우스 이동 중 드래그 시작
            if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
            {
                sf::Vector2f mousePos(static_cast<float>(mouseMoved->position.x),
                                       static_cast<float>(mouseMoved->position.y));

                // 드래그 중이면 호버 슬롯 하이라이트
                if (m_dragDropManager->isDragging())
                {
                    int hoverSlot = getSlotAtPosition(mousePos);
                    for (size_t i = 0; i < m_slots.size(); ++i)
                    {
                        m_slots[i]->setHighlight(static_cast<int>(i) == hoverSlot);
                    }
                }

                if (m_isPotentialDrag && !m_dragDropManager->isDragging())
                {
                    float distance = std::sqrt(
                        std::pow(mousePos.x - m_dragStartPos.x, 2) +
                        std::pow(mousePos.y - m_dragStartPos.y, 2)
                    );

                    if (distance > DRAG_THRESHOLD)
                    {
                        // 드래그 시작
                        OptionalItem item = m_items[m_dragStartSlot];
                        if (item)
                        {
                            m_dragDropManager->startDrag(*item, this, m_dragStartSlot, mousePos);
                            // 원래 슬롯에서 아이템 제거 (데이터 + 시각적)
                            m_items[m_dragStartSlot] = std::nullopt;
                            m_slots[m_dragStartSlot]->setItem(std::nullopt);
                        }
                        m_isPotentialDrag = false;
                        return true;
                    }
                }
            }
        }

        // 스크롤바 드래그 처리 (Window보다 먼저)
        if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
        {
            if (mousePressed->button == sf::Mouse::Button::Left)
            {
                sf::Vector2f mousePos(static_cast<float>(mousePressed->position.x),
                                       static_cast<float>(mousePressed->position.y));
                if (m_scrollThumb.getGlobalBounds().contains(mousePos))
                {
                    m_isDraggingScroll = true;
                    m_scrollDragStart = mousePos.y - m_scrollThumb.getPosition().y;
                    return true;
                }
                // 스크롤바 트랙 클릭 시 해당 위치로 점프
                if (m_scrollbar.getGlobalBounds().contains(mousePos))
                {
                    float minY = m_scrollbar.getPosition().y;
                    float maxY = minY + m_scrollbar.getSize().y - m_scrollThumb.getSize().y;
                    float newThumbY = std::clamp(mousePos.y - m_scrollThumb.getSize().y / 2.f, minY, maxY);

                    float scrollRatio = (maxY > minY) ? (newThumbY - minY) / (maxY - minY) : 0.f;
                    m_scrollOffset = scrollRatio * getMaxScroll();

                    updateSlotPositions();
                    updateScrollThumbPosition();
                    return true;
                }
            }
        }
        else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>())
        {
            if (mouseReleased->button == sf::Mouse::Button::Left && m_isDraggingScroll)
            {
                m_isDraggingScroll = false;
                return true;
            }
        }
        else if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
        {
            if (m_isDraggingScroll)
            {
                float mouseY = static_cast<float>(mouseMoved->position.y);
                float newThumbY = mouseY - m_scrollDragStart;

                float minY = m_scrollbar.getPosition().y;
                float maxY = minY + m_scrollbar.getSize().y - m_scrollThumb.getSize().y;
                newThumbY = std::clamp(newThumbY, minY, maxY);

                float scrollRatio = (maxY > minY) ? (newThumbY - minY) / (maxY - minY) : 0.f;
                m_scrollOffset = scrollRatio * getMaxScroll();

                updateSlotPositions();
                updateScrollThumbPosition();
                return true;
            }

            // 슬롯 호버 업데이트
            sf::Vector2f mousePos(static_cast<float>(mouseMoved->position.x),
                                   static_cast<float>(mouseMoved->position.y));
            for (size_t i = 0; i < m_slots.size(); ++i)
            {
                sf::Vector2f slotPos = m_slots[i]->getPosition();
                if (slotPos.y + SLOT_SIZE > contentPos.y && slotPos.y < contentPos.y + contentSize.y)
                {
                    m_slots[i]->setHovered(m_slots[i]->contains(mousePos));
                }
                else
                {
                    m_slots[i]->clearHover();
                }
            }
        }

        // 윈도우 이벤트 처리 (드래그, 닫기)
        if (m_window.handleEvent(event))
        {
            updateScrollbarPosition();
            updateSlotPositions();
            return true;
        }

        // 마우스 휠 스크롤
        if (const auto* mouseWheel = event.getIf<sf::Event::MouseWheelScrolled>())
        {
            sf::Vector2f mousePos(static_cast<float>(mouseWheel->position.x),
                                   static_cast<float>(mouseWheel->position.y));
            if (contentBounds.contains(mousePos))
            {
                m_scrollOffset -= mouseWheel->delta * 20.f;
                clampScroll();
                updateSlotPositions();
                updateScrollThumbPosition();
                return true;
            }
        }

        return false;
    }

    int getSlotAtPosition(const sf::Vector2f& pos) const
    {
        sf::Vector2f contentPos = m_window.getContentPosition();
        sf::Vector2f contentSize = m_window.getContentSize();

        for (size_t i = 0; i < m_slots.size(); ++i)
        {
            sf::Vector2f slotPos = m_slots[i]->getPosition();
            // 보이는 영역 내에서만
            if (slotPos.y + SLOT_SIZE > contentPos.y && slotPos.y < contentPos.y + contentSize.y)
            {
                if (m_slots[i]->contains(pos))
                {
                    return static_cast<int>(i);
                }
            }
        }
        return -1;
    }

    bool isVisible() const { return m_window.isVisible(); }
    void setVisible(bool visible) { m_window.setVisible(visible); }

    void setPosition(const sf::Vector2f& position)
    {
        m_window.setPosition(position);
        updateScrollbarPosition();
        updateSlotPositions();
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

        // 윈도우 그리기
        target.draw(m_window, states);

        // 클리핑을 위한 뷰 설정
        sf::View originalView = target.getView();
        sf::Vector2f contentPos = m_window.getContentPosition();
        sf::Vector2f contentSize = m_window.getContentSize();

        // 스크롤바 영역 제외한 콘텐츠 영역
        float contentWidth = contentSize.x - 15.f;

        sf::View clipView;
        clipView.setCenter({contentPos.x + contentWidth / 2.f, contentPos.y + contentSize.y / 2.f});
        clipView.setSize({contentWidth, contentSize.y});

        sf::Vector2u windowSize = target.getSize();
        clipView.setViewport(sf::FloatRect(
            {contentPos.x / windowSize.x, contentPos.y / windowSize.y},
            {contentWidth / windowSize.x, contentSize.y / windowSize.y}
        ));

        target.setView(clipView);

        // 슬롯 그리기
        for (const auto& slot : m_slots)
        {
            target.draw(*slot, states);
        }

        target.setView(originalView);

        // 스크롤바 그리기 (클리핑 외부)
        target.draw(m_scrollbar, states);
        target.draw(m_scrollThumb, states);
    }

    static sf::Vector2f calculateWindowSize()
    {
        float width = GRID_COLS * (SLOT_SIZE + SLOT_PADDING) + SLOT_PADDING + 15.f;
        float height = 5 * (SLOT_SIZE + SLOT_PADDING) + SLOT_PADDING + 25.f;
        return {width, height};
    }

    void createSlots()
    {
        m_slots.clear();
        sf::Vector2f contentPos = m_window.getContentPosition();

        for (int row = 0; row < GRID_ROWS; ++row)
        {
            for (int col = 0; col < GRID_COLS; ++col)
            {
                int index = row * GRID_COLS + col;
                float x = contentPos.x + SLOT_PADDING + col * (SLOT_SIZE + SLOT_PADDING);
                float y = contentPos.y + SLOT_PADDING + row * (SLOT_SIZE + SLOT_PADDING) - m_scrollOffset;

                auto slot = std::make_shared<InventorySlot>(
                    sf::Vector2f{x, y},
                    sf::Vector2f{SLOT_SIZE, SLOT_SIZE}
                );

                m_slots.push_back(slot);
            }
        }
    }

    void updateSlotPositions()
    {
        sf::Vector2f contentPos = m_window.getContentPosition();

        for (int row = 0; row < GRID_ROWS; ++row)
        {
            for (int col = 0; col < GRID_COLS; ++col)
            {
                int index = row * GRID_COLS + col;
                float x = contentPos.x + SLOT_PADDING + col * (SLOT_SIZE + SLOT_PADDING);
                float y = contentPos.y + SLOT_PADDING + row * (SLOT_SIZE + SLOT_PADDING) - m_scrollOffset;
                m_slots[index]->setPosition({x, y});
            }
        }
    }

    void updateScrollbarPosition()
    {
        sf::Vector2f contentPos = m_window.getContentPosition();
        sf::Vector2f contentSize = m_window.getContentSize();

        m_scrollbar.setPosition({contentPos.x + contentSize.x - 12.f, contentPos.y + 2.f});
        m_scrollbar.setSize({10.f, contentSize.y - 4.f});

        updateScrollThumbPosition();
    }

    void updateScrollThumbPosition()
    {
        float maxScroll = getMaxScroll();
        float scrollRatio = (maxScroll > 0) ? (m_scrollOffset / maxScroll) : 0.f;

        float trackHeight = m_scrollbar.getSize().y - m_scrollThumb.getSize().y;
        float thumbY = m_scrollbar.getPosition().y + scrollRatio * trackHeight;

        m_scrollThumb.setPosition({m_scrollbar.getPosition().x, thumbY});
    }

    float getMaxScroll() const
    {
        float totalHeight = GRID_ROWS * (SLOT_SIZE + SLOT_PADDING) + SLOT_PADDING;
        float visibleHeight = m_window.getContentSize().y;
        return std::max(0.f, totalHeight - visibleHeight);
    }

    void clampScroll()
    {
        m_scrollOffset = std::clamp(m_scrollOffset, 0.f, getMaxScroll());
    }

    const sf::Font& m_font;
    Window m_window;
    std::vector<std::shared_ptr<InventorySlot>> m_slots;
    std::vector<OptionalItem> m_items;
    DragDropManager* m_dragDropManager = nullptr;

    sf::RectangleShape m_scrollbar;
    sf::RectangleShape m_scrollThumb;
    float m_scrollOffset = 0.f;
    bool m_isDraggingScroll = false;
    float m_scrollDragStart = 0.f;

    // 드래그 관련
    bool m_isPotentialDrag = false;
    sf::Vector2f m_dragStartPos;
    int m_dragStartSlot = -1;
};
