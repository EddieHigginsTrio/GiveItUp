#include "Player.hpp"
#include "TileMap.hpp"
#include <algorithm>

void Player::update(float deltaTime, const TileMap* tileMap)
{
    applyGravity(deltaTime);

    // X축 이동
    sf::Vector2f pos = m_shape.getPosition();
    float newX = pos.x + m_velocity.x * deltaTime;

    // X축 충돌 검사
    if (tileMap)
    {
        sf::FloatRect bounds = m_shape.getGlobalBounds();
        float testX = (m_velocity.x > 0) ? newX + WIDTH : newX;

        // 플레이어의 상단, 중간, 하단에서 충돌 검사
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

        if (collisionX)
        {
            // 벽에 붙이기
            if (m_velocity.x > 0)
            {
                int tileX = static_cast<int>(newX + WIDTH) / TileMap::TILE_SIZE;
                newX = static_cast<float>(tileX * TileMap::TILE_SIZE) - WIDTH;
            }
            else if (m_velocity.x < 0)
            {
                int tileX = static_cast<int>(newX) / TileMap::TILE_SIZE;
                newX = static_cast<float>((tileX + 1) * TileMap::TILE_SIZE);
            }
            m_velocity.x = 0.f;
        }
    }

    m_shape.setPosition({newX, pos.y});

    // Y축 이동
    pos = m_shape.getPosition();
    float newY = pos.y + m_velocity.y * deltaTime;

    // Y축 충돌 검사
    m_isOnGround = false;

    if (tileMap)
    {
        float testY = (m_velocity.y > 0) ? newY + HEIGHT : newY;

        // 플레이어의 왼쪽, 중간, 오른쪽에서 충돌 검사
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

            // 플랫폼 충돌 (아래로 떨어질 때만)
            if (m_velocity.y > 0 && tileMap->isPlatform(tileX, tileY))
            {
                // 이전 위치에서 플랫폼 위에 있었는지 확인
                float platformTop = static_cast<float>(tileY * TileMap::TILE_SIZE);
                if (pos.y + HEIGHT <= platformTop + 5.f)  // 약간의 여유
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
                // 바닥에 착지
                int tileY = static_cast<int>(newY + HEIGHT) / TileMap::TILE_SIZE;
                newY = static_cast<float>(tileY * TileMap::TILE_SIZE) - HEIGHT;
                m_isOnGround = true;
            }
            else if (m_velocity.y < 0)
            {
                // 천장에 부딪힘
                int tileY = static_cast<int>(newY) / TileMap::TILE_SIZE;
                newY = static_cast<float>((tileY + 1) * TileMap::TILE_SIZE);
            }
            m_velocity.y = 0.f;
        }
    }

    m_shape.setPosition({pos.x, newY});
}
