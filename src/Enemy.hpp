#pragma once

#include <SFML/Graphics.hpp>

class TileMap;

class Enemy : public sf::Drawable
{
public:
    static constexpr float WIDTH = 40.f;
    static constexpr float HEIGHT = 40.f;
    static constexpr float MOVE_SPEED = 100.f;
    static constexpr float GRAVITY = 1200.f;
    static constexpr float MAX_FALL_SPEED = 800.f;
    static constexpr float DAMAGE = 10.f;
    static constexpr float KNOCKBACK_FORCE = 400.f;
    static constexpr float KNOCKBACK_DURATION = 0.2f;  // 넉백 지속 시간

    Enemy(const sf::Vector2f& position)
    {
        m_shape.setSize({WIDTH, HEIGHT});
        m_shape.setFillColor(sf::Color{255, 80, 80});
        m_shape.setOutlineThickness(2.f);
        m_shape.setOutlineColor(sf::Color{200, 50, 50});
        m_shape.setPosition(position);
    }

    void update(float deltaTime, const TileMap* tileMap);

    sf::Vector2f getPosition() const { return m_shape.getPosition(); }
    sf::Vector2f getSize() const { return m_shape.getSize(); }
    sf::FloatRect getBounds() const { return m_shape.getGlobalBounds(); }
    sf::Vector2f getCenter() const
    {
        return m_shape.getPosition() + sf::Vector2f(WIDTH / 2.f, HEIGHT / 2.f);
    }

    float getDamage() const { return DAMAGE; }
    float getKnockbackForce() const { return KNOCKBACK_FORCE; }

    bool isAlive() const { return m_isAlive; }
    bool isKnockback() const { return m_isKnockback; }

    void takeDamage(float damage, float knockbackForce, const sf::Vector2f& attackerCenter)
    {
        m_health -= damage;
        if (m_health <= 0.f)
        {
            m_isAlive = false;
            return;
        }

        // 넉백 방향 계산 (공격자 중심에서 적 중심으로)
        sf::Vector2f myCenter = getCenter();
        float direction = (myCenter.x > attackerCenter.x) ? 1.f : -1.f;

        // 넉백 적용
        m_velocity.x = knockbackForce * direction;
        m_velocity.y = -knockbackForce * 0.3f;  // 약간 위로 튀어오름
        m_isKnockback = true;
        m_knockbackTimer = KNOCKBACK_DURATION;
        m_isOnGround = false;

        // 피격 시 색상 변경
        m_shape.setFillColor(sf::Color{255, 200, 200});
    }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        if (m_isAlive)
        {
            target.draw(m_shape, states);
        }
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

    void updateKnockback(float deltaTime)
    {
        if (m_isKnockback)
        {
            m_knockbackTimer -= deltaTime;
            if (m_knockbackTimer <= 0.f)
            {
                m_isKnockback = false;
                // 원래 색상으로 복귀
                m_shape.setFillColor(sf::Color{255, 80, 80});
            }
        }
    }

    sf::RectangleShape m_shape;
    sf::Vector2f m_velocity{0.f, 0.f};
    bool m_isOnGround = false;
    bool m_movingRight = true;
    bool m_isAlive = true;
    float m_health = 30.f;

    // 넉백 관련
    bool m_isKnockback = false;
    float m_knockbackTimer = 0.f;
};
