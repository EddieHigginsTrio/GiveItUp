#pragma once

#include "Window.hpp"
#include "Button.hpp"
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

    InventoryWindow(const sf::Vector2f& position, const sf::Font& font)
        : m_font(font)
        , m_window(position, calculateWindowSize(), font, "Inventory")
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

        updateScrollbarPosition();
        createSlots();
    }

    void setSlotCallback(std::function<void(int)> callback)
    {
        m_slotCallback = std::move(callback);
    }

    void setSlotText(int index, const std::string& text)
    {
        if (index >= 0 && index < static_cast<int>(m_slots.size()))
        {
            m_slots[index]->setText(text);
        }
    }

    bool handleEvent(const sf::Event& event)
    {
        if (!m_window.isVisible()) return false;

        sf::Vector2f contentPos = m_window.getContentPosition();
        sf::Vector2f contentSize = m_window.getContentSize();
        sf::FloatRect contentBounds(contentPos, contentSize);

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

        // 슬롯 버튼 이벤트 (보이는 영역 내에서만)
        for (size_t i = 0; i < m_slots.size(); ++i)
        {
            sf::Vector2f slotPos = m_slots[i]->getPosition();
            if (slotPos.y + SLOT_SIZE > contentPos.y && slotPos.y < contentPos.y + contentSize.y)
            {
                if (m_slots[i]->handleEvent(event))
                {
                    return true;
                }
            }
            else
            {
                m_slots[i]->clearHover();
            }
        }

        return false;
    }

    bool isVisible() const { return m_window.isVisible(); }
    void setVisible(bool visible) { m_window.setVisible(visible); }

    void setPosition(const sf::Vector2f& position)
    {
        m_window.setPosition(position);
        updateScrollbarPosition();
        updateSlotPositions();
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
        float width = GRID_COLS * (SLOT_SIZE + SLOT_PADDING) + SLOT_PADDING + 15.f; // +15 for scrollbar
        float height = 5 * (SLOT_SIZE + SLOT_PADDING) + SLOT_PADDING + 25.f; // 5개 행만 표시 + 타이틀바
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

                auto slot = std::make_shared<Button>(
                    sf::Vector2f{x, y},
                    sf::Vector2f{SLOT_SIZE, SLOT_SIZE},
                    m_font,
                    std::to_string(index + 1)
                );

                slot->setNormalColor(sf::Color{70, 70, 70});
                slot->setHoverColor(sf::Color{90, 90, 90});
                slot->setPressedColor(sf::Color{50, 50, 50});

                int slotIndex = index;
                slot->setCallback([this, slotIndex]() {
                    if (m_slotCallback)
                    {
                        m_slotCallback(slotIndex);
                    }
                });

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
    std::vector<std::shared_ptr<Button>> m_slots;
    std::function<void(int)> m_slotCallback;

    sf::RectangleShape m_scrollbar;
    sf::RectangleShape m_scrollThumb;
    float m_scrollOffset = 0.f;
    bool m_isDraggingScroll = false;
    float m_scrollDragStart = 0.f;
};
