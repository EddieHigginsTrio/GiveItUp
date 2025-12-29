#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class TileMap : public sf::Drawable
{
public:
    static constexpr int TILE_SIZE = 32;

    enum class TileType
    {
        Empty = 0,
        Solid = 1,
        Platform = 2  // 위에서만 충돌 (점프해서 올라갈 수 있음)
    };

    TileMap(int width, int height)
        : m_width(width)
        , m_height(height)
    {
        m_tiles.resize(width * height, TileType::Empty);
    }

    void setTile(int x, int y, TileType type)
    {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
        {
            m_tiles[y * m_width + x] = type;
        }
    }

    TileType getTile(int x, int y) const
    {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
        {
            return m_tiles[y * m_width + x];
        }
        return TileType::Solid;  // 맵 밖은 솔리드로 처리
    }

    TileType getTileAtPosition(float worldX, float worldY) const
    {
        int tileX = static_cast<int>(worldX) / TILE_SIZE;
        int tileY = static_cast<int>(worldY) / TILE_SIZE;
        return getTile(tileX, tileY);
    }

    bool isSolid(int x, int y) const
    {
        TileType type = getTile(x, y);
        return type == TileType::Solid;
    }

    bool isPlatform(int x, int y) const
    {
        return getTile(x, y) == TileType::Platform;
    }

    sf::FloatRect getTileBounds(int x, int y) const
    {
        return sf::FloatRect(
            {static_cast<float>(x * TILE_SIZE), static_cast<float>(y * TILE_SIZE)},
            {static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)}
        );
    }

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getTileSize() const { return TILE_SIZE; }

    // 간단한 레벨 생성
    void createSimpleLevel()
    {
        // 바닥을 타일맵 맨 아래에 배치 (카메라 클램핑으로 항상 보임)
        const int floorOffset = 0;

        // 바닥 생성 (위로 올림)
        for (int x = 0; x < m_width; ++x)
        {
            setTile(x, m_height - 1 - floorOffset, TileType::Solid);
            setTile(x, m_height - 2 - floorOffset, TileType::Solid);
        }

        // 플랫폼들 생성
        // 낮은 플랫폼 (왼쪽)
        for (int x = 3; x < 8; ++x)
        {
            setTile(x, m_height - 5 - floorOffset, TileType::Solid);
        }

        // 중간 플랫폼 (가운데)
        for (int x = 12; x < 18; ++x)
        {
            setTile(x, m_height - 7 - floorOffset, TileType::Solid);
        }

        // 높은 플랫폼 (오른쪽)
        for (int x = 22; x < 28; ++x)
        {
            setTile(x, m_height - 9 - floorOffset, TileType::Solid);
        }

        // 점프 가능 플랫폼 (Platform 타입)
        for (int x = 8; x < 12; ++x)
        {
            setTile(x, m_height - 4 - floorOffset, TileType::Platform);
        }

        // 벽
        for (int y = m_height - 6 - floorOffset; y < m_height - 2 - floorOffset; ++y)
        {
            setTile(30, y, TileType::Solid);
        }
    }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        sf::RectangleShape tileShape;
        tileShape.setSize({static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)});

        for (int y = 0; y < m_height; ++y)
        {
            for (int x = 0; x < m_width; ++x)
            {
                TileType type = getTile(x, y);
                if (type == TileType::Empty)
                    continue;

                tileShape.setPosition({static_cast<float>(x * TILE_SIZE),
                                        static_cast<float>(y * TILE_SIZE)});

                if (type == TileType::Solid)
                {
                    tileShape.setFillColor(sf::Color{80, 60, 40});
                    tileShape.setOutlineThickness(1.f);
                    tileShape.setOutlineColor(sf::Color{100, 80, 60});
                }
                else if (type == TileType::Platform)
                {
                    tileShape.setFillColor(sf::Color{60, 100, 60});
                    tileShape.setOutlineThickness(1.f);
                    tileShape.setOutlineColor(sf::Color{80, 120, 80});
                }

                target.draw(tileShape, states);
            }
        }
    }

    int m_width;
    int m_height;
    std::vector<TileType> m_tiles;
};
