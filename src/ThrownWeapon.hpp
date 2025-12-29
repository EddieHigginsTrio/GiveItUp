#pragma once

#include <SFML/Graphics.hpp>
#include "Item.hpp"

class TileMap;

// 던져진 무기 상태
enum class ThrownWeaponState
{
    Flying,     // 날아가는 중
    Dropped     // 바닥에 떨어짐
};

class ThrownWeapon : public sf::Drawable
{
public:
    static constexpr float THROW_SPEED = 600.f;         // 던지기 속도
    static constexpr float GRAVITY = 800.f;             // 중력
    static constexpr float ROTATION_SPEED = 720.f;      // 회전 속도 (도/초)
    static constexpr float DAMAGE = 25.f;               // 데미지
    static constexpr float KNOCKBACK = 400.f;           // 넉백
    static constexpr float PICKUP_RANGE = 40.f;         // 줍기 범위
    static constexpr float SPRITE_SIZE = 32.f;          // 스프라이트 크기
    static constexpr int SPRITE_WIDTH = 352;            // weapons.png 타일 너비
    static constexpr int SPRITE_HEIGHT = 384;           // weapons.png 타일 높이

    ThrownWeapon(const sf::Vector2f& position, bool facingRight, const Item& weapon, const sf::Texture* texture)
        : m_position(position)
        , m_weapon(weapon)
        , m_texture(texture)
        , m_state(ThrownWeaponState::Flying)
        , m_rotation(0.f)
        , m_hasHitEnemy(false)
    {
        // 던지는 방향에 따라 속도 설정
        m_velocity.x = facingRight ? THROW_SPEED : -THROW_SPEED;
        m_velocity.y = -200.f;  // 약간 위로 던짐
        m_facingRight = facingRight;
    }

    void update(float deltaTime, const TileMap* tileMap);

    // 상태 확인
    ThrownWeaponState getState() const { return m_state; }
    bool isFlying() const { return m_state == ThrownWeaponState::Flying; }
    bool isDropped() const { return m_state == ThrownWeaponState::Dropped; }

    // 충돌 관련
    sf::FloatRect getBounds() const
    {
        return sf::FloatRect(
            {m_position.x - SPRITE_SIZE / 2.f, m_position.y - SPRITE_SIZE / 2.f},
            {SPRITE_SIZE, SPRITE_SIZE}
        );
    }

    sf::Vector2f getPosition() const { return m_position; }
    sf::Vector2f getCenter() const { return m_position; }

    // 플레이어가 줍기 가능한지 확인
    bool canPickup(const sf::Vector2f& playerCenter) const
    {
        if (m_state != ThrownWeaponState::Dropped) return false;

        float dx = playerCenter.x - m_position.x;
        float dy = playerCenter.y - m_position.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        return distance < PICKUP_RANGE;
    }

    // 무기 정보 반환 (줍기용)
    const Item& getWeapon() const { return m_weapon; }

    // 데미지/넉백
    float getDamage() const { return DAMAGE; }
    float getKnockback() const { return KNOCKBACK; }

    // 적에게 맞았는지 확인 (한 번만 데미지)
    bool hasHitEnemy() const { return m_hasHitEnemy; }
    void setHitEnemy() { m_hasHitEnemy = true; }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        if (!m_texture) return;

        sf::IntRect textureRect(
            {m_weapon.spriteX * SPRITE_WIDTH, m_weapon.spriteY * SPRITE_HEIGHT},
            {SPRITE_WIDTH, SPRITE_HEIGHT}
        );

        sf::Sprite sprite(*m_texture, textureRect);
        sprite.setOrigin({SPRITE_WIDTH / 2.f, SPRITE_HEIGHT / 2.f});

        float scale = SPRITE_SIZE / static_cast<float>(SPRITE_WIDTH);
        sprite.setScale({scale, scale});
        sprite.setPosition(m_position);
        sprite.setRotation(sf::degrees(m_rotation));

        // 떨어진 상태면 약간 투명하게
        if (m_state == ThrownWeaponState::Dropped)
        {
            sprite.setColor(sf::Color(255, 255, 255, 200));
        }

        target.draw(sprite, states);
    }

    sf::Vector2f m_position;
    sf::Vector2f m_velocity;
    Item m_weapon;
    const sf::Texture* m_texture;
    ThrownWeaponState m_state;
    float m_rotation;
    bool m_facingRight;
    bool m_hasHitEnemy;
};
