#pragma once

#include <SFML/Graphics.hpp>

class TileMap;

class Player : public sf::Drawable
{
public:
    static constexpr float WIDTH = 32.f;
    static constexpr float HEIGHT = 48.f;
    static constexpr float WALK_SPEED = 200.f;
    static constexpr float JUMP_VELOCITY = -400.f;
    static constexpr float GRAVITY = 980.f;
    static constexpr float MAX_FALL_SPEED = 600.f;

    Player(const sf::Vector2f& position)
    {
        m_shape.setSize({WIDTH, HEIGHT});
        m_shape.setFillColor(sf::Color{100, 150, 255});
        m_shape.setOutlineThickness(2.f);
        m_shape.setOutlineColor(sf::Color::White);
        m_shape.setPosition(position);
    }

    void handleInput()
    {
        m_velocity.x = 0.f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        {
            m_velocity.x = -WALK_SPEED;
            m_facingRight = false;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        {
            m_velocity.x = WALK_SPEED;
            m_facingRight = true;
        }

        if ((sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) ||
             sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
             sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) && m_isOnGround)
        {
            m_velocity.y = JUMP_VELOCITY;
            m_isOnGround = false;
        }
    }

    void update(float deltaTime, const TileMap* tileMap);

    sf::Vector2f getPosition() const { return m_shape.getPosition(); }
    sf::Vector2f getSize() const { return m_shape.getSize(); }
    sf::FloatRect getBounds() const { return m_shape.getGlobalBounds(); }

    bool isOnGround() const { return m_isOnGround; }
    bool isFacingRight() const { return m_facingRight; }
    sf::Vector2f getVelocity() const { return m_velocity; }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(m_shape, states);
    }

    void applyGravity(float deltaTime)
    {
        if (!m_isOnGround)
        {
            m_velocity.y += GRAVITY * deltaTime;
            if (m_velocity.y > MAX_FALL_SPEED)
            {
                m_velocity.y = MAX_FALL_SPEED;
            }
        }
    }

    sf::RectangleShape m_shape;
    sf::Vector2f m_velocity{0.f, 0.f};
    bool m_isOnGround = false;
    bool m_facingRight = true;
};
