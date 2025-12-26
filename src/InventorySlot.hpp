#pragma once

#include <SFML/Graphics.hpp>
#include "Item.hpp"
#include <functional>

class InventorySlot : public sf::Drawable
{
public:
    InventorySlot(const sf::Vector2f& position, const sf::Vector2f& size)
    {
        m_background.setPosition(position);
        m_background.setSize(size);
        m_background.setFillColor(m_normalColor);
        m_background.setOutlineThickness(1.f);
        m_background.setOutlineColor(sf::Color{100, 100, 100});

        m_itemRect.setSize({size.x - 8.f, size.y - 8.f});
        updateItemRectPosition();
    }

    void setItem(const OptionalItem& item)
    {
        m_item = item;
        if (m_item)
        {
            m_itemRect.setFillColor(m_item->color);
        }
    }

    OptionalItem getItem() const { return m_item; }
    bool hasItem() const { return m_item.has_value(); }

    OptionalItem takeItem()
    {
        OptionalItem item = m_item;
        m_item = std::nullopt;
        return item;
    }

    void setPosition(const sf::Vector2f& position)
    {
        m_background.setPosition(position);
        updateItemRectPosition();
    }

    sf::Vector2f getPosition() const
    {
        return m_background.getPosition();
    }

    sf::Vector2f getSize() const
    {
        return m_background.getSize();
    }

    bool contains(const sf::Vector2f& point) const
    {
        return m_background.getGlobalBounds().contains(point);
    }

    void setHighlight(bool highlight)
    {
        m_isHighlighted = highlight;
        updateColor();
    }

    void setHovered(bool hovered)
    {
        m_isHovered = hovered;
        updateColor();
    }

    void clearHover()
    {
        m_isHovered = false;
        updateColor();
    }

    // 드래그 시작 가능 여부 (아이템이 있을 때만)
    bool canStartDrag() const { return m_item.has_value(); }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(m_background, states);
        if (m_item)
        {
            target.draw(m_itemRect, states);
        }
    }

    void updateItemRectPosition()
    {
        sf::Vector2f pos = m_background.getPosition();
        sf::Vector2f size = m_background.getSize();
        m_itemRect.setPosition({pos.x + 4.f, pos.y + 4.f});
    }

    void updateColor()
    {
        if (m_isHighlighted)
            m_background.setFillColor(m_highlightColor);
        else if (m_isHovered)
            m_background.setFillColor(m_hoverColor);
        else
            m_background.setFillColor(m_normalColor);
    }

    sf::RectangleShape m_background;
    sf::RectangleShape m_itemRect;
    OptionalItem m_item;

    bool m_isHovered = false;
    bool m_isHighlighted = false;

    sf::Color m_normalColor{70, 70, 70};
    sf::Color m_hoverColor{90, 90, 90};
    sf::Color m_highlightColor{70, 100, 70};
};
