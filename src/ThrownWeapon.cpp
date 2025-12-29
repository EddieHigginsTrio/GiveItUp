#include "ThrownWeapon.hpp"
#include "TileMap.hpp"

void ThrownWeapon::update(float deltaTime, const TileMap* tileMap)
{
    if (m_state == ThrownWeaponState::Dropped)
    {
        return;  // 떨어진 상태면 업데이트 안 함
    }

    // 중력 적용
    m_velocity.y += GRAVITY * deltaTime;

    // 위치 업데이트
    m_position.x += m_velocity.x * deltaTime;
    m_position.y += m_velocity.y * deltaTime;

    // 회전 (날아가는 동안)
    float rotationDir = m_facingRight ? 1.f : -1.f;
    m_rotation += ROTATION_SPEED * rotationDir * deltaTime;

    // 타일맵 충돌 검사
    if (tileMap)
    {
        // 무기 중심 위치로 타일 충돌 확인
        int tileX = static_cast<int>(m_position.x / tileMap->getTileSize());
        int tileY = static_cast<int>(m_position.y / tileMap->getTileSize());

        // 바닥 충돌 (현재 타일 또는 아래 타일이 solid인지)
        if (tileMap->isSolid(tileX, tileY))
        {
            // 떨어짐 상태로 전환
            m_state = ThrownWeaponState::Dropped;
            m_velocity = {0.f, 0.f};

            // 회전 각도를 90도로 (바닥에 누워있는 모습)
            m_rotation = 90.f;
        }
    }

    // 화면 밖으로 나가면 떨어짐 상태로
    if (m_position.y > 2000.f)
    {
        m_state = ThrownWeaponState::Dropped;
    }
}
