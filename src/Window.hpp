#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

class Window : public sf::Drawable
{
public:
    Window(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Font& font, const std::string& title)
        : m_font(font)
        , m_titleText(font, title, 16)
        , m_closeText(font, "X", 14)
    {
        m_size = size;

        // 타이틀바
        m_titleBar.setPosition(position);
        m_titleBar.setSize({size.x, m_titleBarHeight});
        m_titleBar.setFillColor(m_titleBarColor);

        // 본체
        m_body.setPosition({position.x, position.y + m_titleBarHeight});
        m_body.setSize({size.x, size.y - m_titleBarHeight});
        m_body.setFillColor(m_bodyColor);
        m_body.setOutlineThickness(1.f);
        m_body.setOutlineColor(sf::Color{80, 80, 80});

        // 닫기 버튼
        m_closeButton.setSize({m_titleBarHeight - 4.f, m_titleBarHeight - 4.f});
        m_closeButton.setFillColor(m_closeButtonColor);
        updateCloseButtonPosition();

        // 타이틀 텍스트
        m_titleText.setFillColor(sf::Color::White);
        updateTitlePosition();

        // 닫기 버튼 텍스트
        m_closeText.setFillColor(sf::Color::White);
        updateCloseTextPosition();
    }

    bool handleEvent(const sf::Event& event)
    {
        if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
        {
            sf::Vector2f mousePos(static_cast<float>(mouseMoved->position.x),
                                   static_cast<float>(mouseMoved->position.y));

            // 드래그 중이면 윈도우 이동
            if (m_isDragging)
            {
                sf::Vector2f delta = mousePos - m_dragOffset;
                setPosition(delta);
                return true;
            }

            // 닫기 버튼 호버
            m_isCloseHovered = m_closeButton.getGlobalBounds().contains(mousePos);
            updateCloseButtonColor();

            // 타이틀바 호버
            m_isTitleBarHovered = m_titleBar.getGlobalBounds().contains(mousePos) && !m_isCloseHovered;

            return contains(mousePos);
        }
        else if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
        {
            if (mousePressed->button == sf::Mouse::Button::Left)
            {
                sf::Vector2f mousePos(static_cast<float>(mousePressed->position.x),
                                       static_cast<float>(mousePressed->position.y));

                // 닫기 버튼 클릭
                if (m_closeButton.getGlobalBounds().contains(mousePos))
                {
                    m_isClosePressed = true;
                    return true;
                }

                // 타이틀바 드래그 시작
                if (m_titleBar.getGlobalBounds().contains(mousePos))
                {
                    m_isDragging = true;
                    m_dragOffset = mousePos - m_titleBar.getPosition();
                    return true;
                }

                // 본체 클릭
                if (m_body.getGlobalBounds().contains(mousePos))
                {
                    return true;
                }
            }
        }
        else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>())
        {
            if (mouseReleased->button == sf::Mouse::Button::Left)
            {
                sf::Vector2f mousePos(static_cast<float>(mouseReleased->position.x),
                                       static_cast<float>(mouseReleased->position.y));

                if (m_isDragging)
                {
                    m_isDragging = false;
                    return true;
                }

                if (m_isClosePressed)
                {
                    m_isClosePressed = false;
                    if (m_closeButton.getGlobalBounds().contains(mousePos) && m_onClose)
                    {
                        m_onClose();
                    }
                    return true;
                }
            }
        }
        return false;
    }

    void setPosition(const sf::Vector2f& position)
    {
        m_titleBar.setPosition(position);
        m_body.setPosition({position.x, position.y + m_titleBarHeight});
        updateCloseButtonPosition();
        updateTitlePosition();
        updateCloseTextPosition();
    }

    sf::Vector2f getPosition() const
    {
        return m_titleBar.getPosition();
    }

    sf::Vector2f getContentPosition() const
    {
        return m_body.getPosition();
    }

    sf::Vector2f getContentSize() const
    {
        return m_body.getSize();
    }

    void setOnClose(std::function<void()> callback)
    {
        m_onClose = std::move(callback);
    }

    void setTitle(const std::string& title)
    {
        m_titleText.setString(title);
        updateTitlePosition();
    }

    bool contains(const sf::Vector2f& point) const
    {
        return m_titleBar.getGlobalBounds().contains(point) ||
               m_body.getGlobalBounds().contains(point);
    }

    bool isVisible() const { return m_isVisible; }
    void setVisible(bool visible) { m_isVisible = visible; }

    void setTitleBarColor(const sf::Color& color) { m_titleBarColor = color; m_titleBar.setFillColor(color); }
    void setBodyColor(const sf::Color& color) { m_bodyColor = color; m_body.setFillColor(color); }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        if (!m_isVisible) return;

        target.draw(m_body, states);
        target.draw(m_titleBar, states);
        target.draw(m_closeButton, states);
        target.draw(m_titleText, states);
        target.draw(m_closeText, states);
    }

    void updateCloseButtonPosition()
    {
        sf::Vector2f titlePos = m_titleBar.getPosition();
        m_closeButton.setPosition({
            titlePos.x + m_size.x - m_titleBarHeight + 2.f,
            titlePos.y + 2.f
        });
    }

    void updateTitlePosition()
    {
        sf::FloatRect textBounds = m_titleText.getLocalBounds();
        sf::Vector2f titlePos = m_titleBar.getPosition();
        m_titleText.setPosition({
            titlePos.x + 8.f,
            titlePos.y + (m_titleBarHeight - textBounds.size.y) / 2.f - textBounds.position.y
        });
    }

    void updateCloseTextPosition()
    {
        sf::FloatRect textBounds = m_closeText.getLocalBounds();
        sf::FloatRect buttonBounds = m_closeButton.getGlobalBounds();
        m_closeText.setPosition({
            buttonBounds.position.x + (buttonBounds.size.x - textBounds.size.x) / 2.f - textBounds.position.x,
            buttonBounds.position.y + (buttonBounds.size.y - textBounds.size.y) / 2.f - textBounds.position.y
        });
    }

    void updateCloseButtonColor()
    {
        if (m_isClosePressed)
            m_closeButton.setFillColor(sf::Color{150, 50, 50});
        else if (m_isCloseHovered)
            m_closeButton.setFillColor(sf::Color{200, 70, 70});
        else
            m_closeButton.setFillColor(m_closeButtonColor);
    }

    const sf::Font& m_font;
    sf::Vector2f m_size;
    float m_titleBarHeight = 25.f;

    sf::RectangleShape m_titleBar;
    sf::RectangleShape m_body;
    sf::RectangleShape m_closeButton;
    sf::Text m_titleText;
    sf::Text m_closeText;

    std::function<void()> m_onClose;

    bool m_isDragging = false;
    sf::Vector2f m_dragOffset;

    bool m_isTitleBarHovered = false;
    bool m_isCloseHovered = false;
    bool m_isClosePressed = false;
    bool m_isVisible = true;

    sf::Color m_titleBarColor{60, 60, 60};
    sf::Color m_bodyColor{40, 40, 40};
    sf::Color m_closeButtonColor{80, 80, 80};
};
