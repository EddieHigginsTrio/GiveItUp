#pragma once

#include <SFML/Graphics.hpp>
#include <deque>

class TileMap;

class Player : public sf::Drawable
{
public:
    static constexpr float WIDTH = 32.f;
    static constexpr float HEIGHT = 48.f;
    static constexpr float WALK_SPEED = 350.f;
    static constexpr float JUMP_VELOCITY = -500.f;
    static constexpr float GRAVITY = 1200.f;
    static constexpr float MAX_FALL_SPEED = 800.f;
    static constexpr float DASH_SPEED = 900.f;
    static constexpr float DASH_DURATION = 0.15f;
    static constexpr float DASH_COOLDOWN = 0.5f;
    static constexpr float AFTERIMAGE_INTERVAL = 0.02f;  // 잔상 생성 간격
    static constexpr float AFTERIMAGE_LIFETIME = 0.15f;  // 잔상 지속 시간
    static constexpr int MAX_AFTERIMAGES = 8;            // 최대 잔상 개수
    static constexpr float KNOCKBACK_DURATION = 0.3f;    // 넉백 지속 시간
    static constexpr float INVINCIBLE_DURATION = 1.0f;   // 무적 시간
    static constexpr float ATTACK_DURATION = 0.2f;       // 공격 지속 시간
    static constexpr float ATTACK_COOLDOWN = 0.3f;       // 공격 쿨다운
    static constexpr float ATTACK_DAMAGE = 20.f;         // 공격력
    static constexpr float ATTACK_KNOCKBACK = 300.f;     // 공격 넉백
    static constexpr float ATTACK_RANGE_X = 60.f;        // 공격 범위 (가로)
    static constexpr float ATTACK_RANGE_Y = 40.f;        // 공격 범위 (세로)

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
        // 대쉬 중이거나 넉백 중에는 입력 무시
        if (m_isDashing || m_isKnockback)
        {
            return;
        }

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

        // Shift 키로 대쉬
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) && m_dashCooldownTimer <= 0.f)
        {
            startDash();
        }

        // Z 키로 공격
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z) && m_attackCooldownTimer <= 0.f && !m_isAttacking)
        {
            startAttack();
        }
    }

    void update(float deltaTime, const TileMap* tileMap);

    sf::Vector2f getPosition() const { return m_shape.getPosition(); }
    sf::Vector2f getSize() const { return m_shape.getSize(); }
    sf::FloatRect getBounds() const { return m_shape.getGlobalBounds(); }

    bool isOnGround() const { return m_isOnGround; }
    bool isFacingRight() const { return m_facingRight; }
    bool isDashing() const { return m_isDashing; }
    bool isInvincible() const { return m_invincibleTimer > 0.f; }
    bool isKnockback() const { return m_isKnockback; }
    sf::Vector2f getVelocity() const { return m_velocity; }

    sf::Vector2f getCenter() const
    {
        return m_shape.getPosition() + sf::Vector2f(WIDTH / 2.f, HEIGHT / 2.f);
    }

    // 피격 처리
    void takeHit(float damage, float knockbackForce, const sf::Vector2f& enemyCenter)
    {
        if (m_invincibleTimer > 0.f || m_isDashing) return;  // 무적 또는 대쉬 중이면 무시

        m_health -= damage;

        // 넉백 방향 계산 (적 중심에서 플레이어 중심으로)
        sf::Vector2f playerCenter = getCenter();
        float direction = (playerCenter.x > enemyCenter.x) ? 1.f : -1.f;

        // 넉백 적용
        m_velocity.x = knockbackForce * direction;
        m_velocity.y = -knockbackForce * 0.5f;  // 약간 위로 튀어오름
        m_isKnockback = true;
        m_knockbackTimer = KNOCKBACK_DURATION;
        m_invincibleTimer = INVINCIBLE_DURATION;
        m_isOnGround = false;

        // 피격 시 빨간색으로 깜빡임
        m_shape.setFillColor(sf::Color{255, 100, 100});
    }

    float getHealth() const { return m_health; }
    float getMaxHealth() const { return m_maxHealth; }

    // 공격 관련
    bool isAttacking() const { return m_isAttacking; }
    float getAttackDamage() const { return ATTACK_DAMAGE; }
    float getAttackKnockback() const { return ATTACK_KNOCKBACK; }

    // 공격 히트박스 반환
    sf::FloatRect getAttackHitbox() const
    {
        if (!m_isAttacking) return sf::FloatRect({0, 0}, {0, 0});

        sf::Vector2f pos = m_shape.getPosition();
        float hitboxX = m_facingRight ? pos.x + WIDTH : pos.x - ATTACK_RANGE_X;
        float hitboxY = pos.y + (HEIGHT - ATTACK_RANGE_Y) / 2.f;

        return sf::FloatRect({hitboxX, hitboxY}, {ATTACK_RANGE_X, ATTACK_RANGE_Y});
    }

private:
    void startAttack()
    {
        m_isAttacking = true;
        m_attackTimer = ATTACK_DURATION;
        m_attackCooldownTimer = ATTACK_COOLDOWN;
        m_hasHitEnemy = false;  // 이번 공격에서 아직 적을 안 맞춤
    }

    void updateAttack(float deltaTime)
    {
        if (m_isAttacking)
        {
            m_attackTimer -= deltaTime;
            if (m_attackTimer <= 0.f)
            {
                m_isAttacking = false;
            }
        }

        if (m_attackCooldownTimer > 0.f)
        {
            m_attackCooldownTimer -= deltaTime;
        }
    }

    void startDash()
    {
        m_isDashing = true;
        m_dashTimer = DASH_DURATION;
        m_dashCooldownTimer = DASH_COOLDOWN;
        m_afterimageTimer = 0.f;  // 즉시 첫 잔상 생성
        m_velocity.x = m_facingRight ? DASH_SPEED : -DASH_SPEED;
        m_velocity.y = 0.f;  // 대쉬 중 수직 속도 초기화
        m_shape.setFillColor(sf::Color{255, 200, 100});  // 대쉬 중 색상 변경
    }

    void updateDash(float deltaTime)
    {
        if (m_isDashing)
        {
            m_dashTimer -= deltaTime;

            // 잔상 생성
            m_afterimageTimer -= deltaTime;
            if (m_afterimageTimer <= 0.f)
            {
                createAfterimage();
                m_afterimageTimer = AFTERIMAGE_INTERVAL;
            }

            if (m_dashTimer <= 0.f)
            {
                m_isDashing = false;
                m_shape.setFillColor(sf::Color{100, 150, 255});  // 원래 색상으로 복귀
            }
        }

        if (m_dashCooldownTimer > 0.f)
        {
            m_dashCooldownTimer -= deltaTime;
        }

        // 잔상 업데이트
        updateAfterimages(deltaTime);

        // 넉백 업데이트
        updateKnockback(deltaTime);

        // 무적 시간 업데이트
        updateInvincibility(deltaTime);

        // 공격 업데이트
        updateAttack(deltaTime);
    }

    void updateKnockback(float deltaTime)
    {
        if (m_isKnockback)
        {
            m_knockbackTimer -= deltaTime;
            if (m_knockbackTimer <= 0.f)
            {
                m_isKnockback = false;
            }
        }
    }

    void updateInvincibility(float deltaTime)
    {
        if (m_invincibleTimer > 0.f)
        {
            m_invincibleTimer -= deltaTime;

            // 무적 중 깜빡임 효과
            float blinkRate = 10.f;
            bool visible = static_cast<int>(m_invincibleTimer * blinkRate) % 2 == 0;

            if (m_invincibleTimer <= 0.f)
            {
                // 무적 종료, 원래 색상으로
                m_shape.setFillColor(sf::Color{100, 150, 255});
            }
            else if (!m_isDashing)
            {
                // 깜빡임 (대쉬 중이 아닐 때만)
                sf::Color color = visible ? sf::Color{100, 150, 255} : sf::Color{255, 150, 150};
                m_shape.setFillColor(color);
            }
        }
    }

    void createAfterimage()
    {
        Afterimage img;
        img.shape.setSize({WIDTH, HEIGHT});
        img.shape.setPosition(m_shape.getPosition());
        img.shape.setFillColor(sf::Color{255, 200, 100, 150});
        img.shape.setOutlineThickness(0.f);
        img.lifetime = AFTERIMAGE_LIFETIME;

        m_afterimages.push_back(img);

        // 최대 개수 초과 시 가장 오래된 것 제거
        while (m_afterimages.size() > MAX_AFTERIMAGES)
        {
            m_afterimages.pop_front();
        }
    }

    void updateAfterimages(float deltaTime)
    {
        for (auto& img : m_afterimages)
        {
            img.lifetime -= deltaTime;
            // 시간에 따라 투명해지게
            float alpha = (img.lifetime / AFTERIMAGE_LIFETIME) * 150.f;
            sf::Color color = img.shape.getFillColor();
            color.a = static_cast<std::uint8_t>(std::max(0.f, alpha));
            img.shape.setFillColor(color);
        }

        // 수명이 다한 잔상 제거
        while (!m_afterimages.empty() && m_afterimages.front().lifetime <= 0.f)
        {
            m_afterimages.pop_front();
        }
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        // 잔상 먼저 그리기 (플레이어 뒤에)
        for (const auto& img : m_afterimages)
        {
            target.draw(img.shape, states);
        }
        target.draw(m_shape, states);

        // 공격 히트박스 시각화
        if (m_isAttacking)
        {
            sf::FloatRect hitbox = getAttackHitbox();
            sf::RectangleShape attackVisual({hitbox.size.x, hitbox.size.y});
            attackVisual.setPosition({hitbox.position.x, hitbox.position.y});
            attackVisual.setFillColor(sf::Color{255, 255, 100, 100});  // 반투명 노란색
            attackVisual.setOutlineThickness(2.f);
            attackVisual.setOutlineColor(sf::Color{255, 200, 50});
            target.draw(attackVisual, states);
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

    // 잔상 구조체
    struct Afterimage
    {
        sf::RectangleShape shape;
        float lifetime = 0.f;
    };

    sf::RectangleShape m_shape;
    sf::Vector2f m_velocity{0.f, 0.f};
    bool m_isOnGround = false;
    bool m_facingRight = true;
    bool m_isDashing = false;
    float m_dashTimer = 0.f;
    float m_dashCooldownTimer = 0.f;
    float m_afterimageTimer = 0.f;
    std::deque<Afterimage> m_afterimages;

    // 넉백 및 무적 관련
    bool m_isKnockback = false;
    float m_knockbackTimer = 0.f;
    float m_invincibleTimer = 0.f;

    // 체력
    float m_health = 100.f;
    float m_maxHealth = 100.f;

    // 공격 관련
    bool m_isAttacking = false;
    float m_attackTimer = 0.f;
    float m_attackCooldownTimer = 0.f;
    bool m_hasHitEnemy = false;
};
