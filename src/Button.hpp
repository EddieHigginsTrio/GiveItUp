#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

class Button : public sf::Drawable
{
public:
    Button(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Font& font, const std::string& text)
        : m_font(font)
        , m_text(font, text, 24)
    {
        m_shape.setPosition(position);
        m_shape.setSize(size);
        m_shape.setFillColor(m_normalColor);
        m_shape.setOutlineThickness(2.f);
        m_shape.setOutlineColor(sf::Color::White);

        m_text.setFillColor(sf::Color::White);
        centerText();
    }

    void setCallback(std::function<void()> callback)
    {
        m_callback = std::move(callback);
    }

    // 이벤트 처리. 이벤트를 소비했으면 true 반환
    bool handleEvent(const sf::Event& event)
    {
        if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
        {
            sf::Vector2f mousePos(static_cast<float>(mouseMoved->position.x),
                                   static_cast<float>(mouseMoved->position.y));
            m_isHovered = m_shape.getGlobalBounds().contains(mousePos);
            updateColor();
            return m_isHovered;
        }
        else if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
        {
            if (mousePressed->button == sf::Mouse::Button::Left)
            {
                sf::Vector2f mousePos(static_cast<float>(mousePressed->position.x),
                                       static_cast<float>(mousePressed->position.y));
                if (m_shape.getGlobalBounds().contains(mousePos))
                {
                    m_isPressed = true;
                    updateColor();
                    return true;
                }
            }
        }
        else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>())
        {
            if (mouseReleased->button == sf::Mouse::Button::Left && m_isPressed)
            {
                m_isPressed = false;
                sf::Vector2f mousePos(static_cast<float>(mouseReleased->position.x),
                                       static_cast<float>(mouseReleased->position.y));
                if (m_shape.getGlobalBounds().contains(mousePos) && m_callback)
                {
                    m_callback();
                }
                updateColor();
                return true;
            }
        }
        return false;
    }

    // 호버 상태 강제 해제 (다른 버튼이 이벤트를 가져갔을 때)
    void clearHover()
    {
        m_isHovered = false;
        updateColor();
    }

    bool contains(const sf::Vector2f& point) const
    {
        return m_shape.getGlobalBounds().contains(point);
    }

    void setText(const std::string& text)
    {
        m_text.setString(text);
        centerText();
    }

    void setPosition(const sf::Vector2f& position)
    {
        m_shape.setPosition(position);
        centerText();
    }

    sf::Vector2f getPosition() const
    {
        return m_shape.getPosition();
    }

    sf::Vector2f getSize() const
    {
        return m_shape.getSize();
    }

    void setNormalColor(const sf::Color& color) { m_normalColor = color; updateColor(); }
    void setHoverColor(const sf::Color& color) { m_hoverColor = color; updateColor(); }
    void setPressedColor(const sf::Color& color) { m_pressedColor = color; updateColor(); }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(m_shape, states);
        target.draw(m_text, states);
    }

    void centerText()
    {
        sf::FloatRect textBounds = m_text.getLocalBounds();
        sf::FloatRect shapeBounds = m_shape.getGlobalBounds();
        m_text.setPosition({
            shapeBounds.position.x + (shapeBounds.size.x - textBounds.size.x) / 2.f - textBounds.position.x,
            shapeBounds.position.y + (shapeBounds.size.y - textBounds.size.y) / 2.f - textBounds.position.y
        });
    }

    void updateColor()
    {
        if (m_isPressed)
            m_shape.setFillColor(m_pressedColor);
        else if (m_isHovered)
            m_shape.setFillColor(m_hoverColor);
        else
            m_shape.setFillColor(m_normalColor);
    }

    sf::RectangleShape m_shape;
    const sf::Font& m_font;
    sf::Text m_text;
    std::function<void()> m_callback;

    bool m_isHovered = false;
    bool m_isPressed = false;

    sf::Color m_normalColor{100, 100, 100};
    sf::Color m_hoverColor{150, 150, 150};
    sf::Color m_pressedColor{70, 70, 70};
};
