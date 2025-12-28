#include "Enemy.hpp"
#include "TileMap.hpp"

void Enemy::update(float deltaTime, const TileMap* tileMap)
{
    if (!m_isAlive) return;

    updateKnockback(deltaTime);
    applyGravity(deltaTime);

    // 넉백 중에는 패트롤 이동 안함
    if (!m_isKnockback)
    {
        // 좌우 패트롤 이동
        m_velocity.x = m_movingRight ? MOVE_SPEED : -MOVE_SPEED;
    }

    // X축 이동
    sf::Vector2f pos = m_shape.getPosition();
    float newX = pos.x + m_velocity.x * deltaTime;

    // X축 충돌 검사
    if (tileMap)
    {
        float testX = m_movingRight ? newX + WIDTH : newX;

        bool collisionX = false;
        for (float testY : {pos.y + 1.f, pos.y + HEIGHT / 2.f, pos.y + HEIGHT - 1.f})
        {
            int tileX = static_cast<int>(testX) / TileMap::TILE_SIZE;
            int tileY = static_cast<int>(testY) / TileMap::TILE_SIZE;

            if (tileMap->isSolid(tileX, tileY))
            {
                collisionX = true;
                break;
            }
        }

        // 벽에 부딪히면 방향 전환
        if (collisionX)
        {
            m_movingRight = !m_movingRight;
            newX = pos.x;
        }

        // 낭떠러지 감지 (앞에 바닥이 없으면 방향 전환)
        if (m_isOnGround)
        {
            float checkX = m_movingRight ? pos.x + WIDTH + 5.f : pos.x - 5.f;
            int tileX = static_cast<int>(checkX) / TileMap::TILE_SIZE;
            int tileY = static_cast<int>(pos.y + HEIGHT + 5.f) / TileMap::TILE_SIZE;

            if (!tileMap->isSolid(tileX, tileY) && !tileMap->isPlatform(tileX, tileY))
            {
                m_movingRight = !m_movingRight;
                newX = pos.x;
            }
        }
    }

    m_shape.setPosition({newX, pos.y});

    // Y축 이동
    pos = m_shape.getPosition();
    float newY = pos.y + m_velocity.y * deltaTime;

    m_isOnGround = false;

    if (tileMap)
    {
        float testY = (m_velocity.y > 0) ? newY + HEIGHT : newY;

        bool collisionY = false;
        for (float testX : {pos.x + 1.f, pos.x + WIDTH / 2.f, pos.x + WIDTH - 1.f})
        {
            int tileX = static_cast<int>(testX) / TileMap::TILE_SIZE;
            int tileY = static_cast<int>(testY) / TileMap::TILE_SIZE;

            if (tileMap->isSolid(tileX, tileY))
            {
                collisionY = true;
                break;
            }

            // 플랫폼 충돌
            if (m_velocity.y > 0 && tileMap->isPlatform(tileX, tileY))
            {
                float platformTop = static_cast<float>(tileY * TileMap::TILE_SIZE);
                if (pos.y + HEIGHT <= platformTop + 5.f)
                {
                    collisionY = true;
                    break;
                }
            }
        }

        if (collisionY)
        {
            if (m_velocity.y > 0)
            {
                int tileY = static_cast<int>(newY + HEIGHT) / TileMap::TILE_SIZE;
                newY = static_cast<float>(tileY * TileMap::TILE_SIZE) - HEIGHT;
                m_isOnGround = true;
            }
            else if (m_velocity.y < 0)
            {
                int tileY = static_cast<int>(newY) / TileMap::TILE_SIZE;
                newY = static_cast<float>((tileY + 1) * TileMap::TILE_SIZE);
            }
            m_velocity.y = 0.f;
        }
    }

    m_shape.setPosition({pos.x, newY});
}
