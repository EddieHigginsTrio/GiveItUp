#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <cstdint>

class TileMap : public sf::Drawable
{
public:
    static constexpr int TILE_SIZE = 32;

    // 바이너리 파일 매직 넘버 및 버전
    static constexpr char FILE_MAGIC[4] = {'T', 'M', 'A', 'P'};
    static constexpr uint16_t FILE_VERSION = 2;  // Version 2: CollisionShape 추가
    static constexpr uint16_t FILE_VERSION_1 = 1;  // 이전 버전 호환용

    enum class TileType
    {
        Empty = 0,
        Solid = 1,
        Platform = 2  // 위에서만 충돌 (점프해서 올라갈 수 있음)
    };

    // 충돌 형태 (경사면 및 특수 충돌)
    enum class CollisionShape : uint8_t
    {
        None = 0,           // 충돌 없음 (Empty 타일용)
        Full = 1,           // 전체 타일 충돌 (기본 Solid)
        SlopeLeftUp = 2,    // 왼쪽 아래 → 오른쪽 위 (/)
        SlopeRightUp = 3,   // 오른쪽 아래 → 왼쪽 위 (\)
        HalfTop = 4,        // 상단 절반만 충돌
        HalfBottom = 5,     // 하단 절반만 충돌
        HalfLeft = 6,       // 좌측 절반만 충돌
        HalfRight = 7,      // 우측 절반만 충돌
        Platform = 8        // 플랫폼 (위에서만 충돌)
    };

    // 타일 데이터 구조
    struct TileData
    {
        TileType type = TileType::Empty;
        CollisionShape shape = CollisionShape::None;
    };

    TileMap(int width, int height)
        : m_width(width)
        , m_height(height)
    {
        m_tiles.resize(width * height);
    }

    void setTile(int x, int y, TileType type)
    {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
        {
            m_tiles[y * m_width + x].type = type;
            // 타입에 따라 기본 충돌 형태 설정
            if (type == TileType::Empty) {
                m_tiles[y * m_width + x].shape = CollisionShape::None;
            } else if (type == TileType::Platform) {
                m_tiles[y * m_width + x].shape = CollisionShape::Platform;
            } else {
                m_tiles[y * m_width + x].shape = CollisionShape::Full;
            }
        }
    }

    void setTileShape(int x, int y, CollisionShape shape)
    {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
        {
            m_tiles[y * m_width + x].shape = shape;
        }
    }

    TileType getTile(int x, int y) const
    {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
        {
            return m_tiles[y * m_width + x].type;
        }
        return TileType::Solid;  // 맵 밖은 솔리드로 처리
    }

    CollisionShape getTileShape(int x, int y) const
    {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
        {
            return m_tiles[y * m_width + x].shape;
        }
        return CollisionShape::Full;  // 맵 밖은 Full로 처리
    }

    const TileData& getTileData(int x, int y) const
    {
        static TileData solidTile = {TileType::Solid, CollisionShape::Full};
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
        {
            return m_tiles[y * m_width + x];
        }
        return solidTile;  // 맵 밖은 솔리드로 처리
    }

    TileType getTileAtPosition(float worldX, float worldY) const
    {
        int tileX = static_cast<int>(worldX) / TILE_SIZE;
        int tileY = static_cast<int>(worldY) / TILE_SIZE;
        return getTile(tileX, tileY);
    }

    CollisionShape getTileShapeAtPosition(float worldX, float worldY) const
    {
        int tileX = static_cast<int>(worldX) / TILE_SIZE;
        int tileY = static_cast<int>(worldY) / TILE_SIZE;
        return getTileShape(tileX, tileY);
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

    // 경사면 타일인지 확인
    bool isSlope(int x, int y) const
    {
        CollisionShape shape = getTileShape(x, y);
        return shape == CollisionShape::SlopeLeftUp || shape == CollisionShape::SlopeRightUp;
    }

    // 경사면에서의 y 좌표 계산 (캐릭터가 경사면 위에 있을 때)
    float getSlopeY(int tileX, int tileY, float worldX) const
    {
        CollisionShape shape = getTileShape(tileX, tileY);
        float tileLeft = static_cast<float>(tileX * TILE_SIZE);
        float tileBottom = static_cast<float>((tileY + 1) * TILE_SIZE);
        float relX = worldX - tileLeft;
        float progress = relX / TILE_SIZE;  // 0.0 ~ 1.0

        if (shape == CollisionShape::SlopeLeftUp) {
            // 왼쪽 아래 → 오른쪽 위 (/)
            // 왼쪽 끝: tileBottom, 오른쪽 끝: tileBottom - TILE_SIZE
            return tileBottom - (progress * TILE_SIZE);
        } else if (shape == CollisionShape::SlopeRightUp) {
            // 오른쪽 아래 → 왼쪽 위 (\)
            // 왼쪽 끝: tileBottom - TILE_SIZE, 오른쪽 끝: tileBottom
            return tileBottom - ((1.0f - progress) * TILE_SIZE);
        }
        return tileBottom;
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

    // 스폰 위치 설정/가져오기
    void setPlayerSpawn(int x, int y) { m_playerSpawnX = x; m_playerSpawnY = y; }
    sf::Vector2i getPlayerSpawn() const { return {m_playerSpawnX, m_playerSpawnY}; }

    void addEnemySpawn(int x, int y, uint8_t type = 0) {
        m_enemySpawns.push_back({x, y, type});
    }
    const std::vector<std::tuple<int, int, uint8_t>>& getEnemySpawns() const { return m_enemySpawns; }
    void clearEnemySpawns() { m_enemySpawns.clear(); }

    // 바이너리 파일 저장
    bool saveToFile(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        // Header
        file.write(FILE_MAGIC, 4);
        file.write(reinterpret_cast<const char*>(&FILE_VERSION), sizeof(FILE_VERSION));
        uint16_t gridSize = TILE_SIZE;
        file.write(reinterpret_cast<const char*>(&gridSize), sizeof(gridSize));
        uint32_t width = m_width;
        uint32_t height = m_height;
        file.write(reinterpret_cast<const char*>(&width), sizeof(width));
        file.write(reinterpret_cast<const char*>(&height), sizeof(height));

        // Tiles (비어있지 않은 타일만 저장)
        std::vector<std::tuple<uint16_t, uint16_t, uint8_t>> nonEmptyTiles;
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                TileType type = getTile(x, y);
                if (type != TileType::Empty) {
                    nonEmptyTiles.push_back({static_cast<uint16_t>(x),
                                              static_cast<uint16_t>(y),
                                              static_cast<uint8_t>(type)});
                }
            }
        }
        uint32_t tileCount = static_cast<uint32_t>(nonEmptyTiles.size());
        file.write(reinterpret_cast<const char*>(&tileCount), sizeof(tileCount));
        for (const auto& [tx, ty, tt] : nonEmptyTiles) {
            file.write(reinterpret_cast<const char*>(&tx), sizeof(tx));
            file.write(reinterpret_cast<const char*>(&ty), sizeof(ty));
            file.write(reinterpret_cast<const char*>(&tt), sizeof(tt));
        }

        // Player Spawn
        int32_t spawnX = m_playerSpawnX;
        int32_t spawnY = m_playerSpawnY;
        file.write(reinterpret_cast<const char*>(&spawnX), sizeof(spawnX));
        file.write(reinterpret_cast<const char*>(&spawnY), sizeof(spawnY));

        // Enemy Spawns
        uint32_t enemyCount = static_cast<uint32_t>(m_enemySpawns.size());
        file.write(reinterpret_cast<const char*>(&enemyCount), sizeof(enemyCount));
        for (const auto& [ex, ey, et] : m_enemySpawns) {
            int32_t enemyX = ex;
            int32_t enemyY = ey;
            file.write(reinterpret_cast<const char*>(&enemyX), sizeof(enemyX));
            file.write(reinterpret_cast<const char*>(&enemyY), sizeof(enemyY));
            file.write(reinterpret_cast<const char*>(&et), sizeof(et));
        }

        return true;
    }

    // 바이너리 파일 로드
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        // Header
        char magic[4];
        file.read(magic, 4);
        if (magic[0] != FILE_MAGIC[0] || magic[1] != FILE_MAGIC[1] ||
            magic[2] != FILE_MAGIC[2] || magic[3] != FILE_MAGIC[3]) {
            return false;
        }

        uint16_t version;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        if (version != FILE_VERSION && version != FILE_VERSION_1) return false;

        uint16_t gridSize;
        file.read(reinterpret_cast<char*>(&gridSize), sizeof(gridSize));
        // gridSize는 현재 TILE_SIZE와 같아야 함 (다르면 스케일링 필요)

        uint32_t width, height;
        file.read(reinterpret_cast<char*>(&width), sizeof(width));
        file.read(reinterpret_cast<char*>(&height), sizeof(height));

        // 맵 크기 재설정
        m_width = static_cast<int>(width);
        m_height = static_cast<int>(height);
        m_tiles.clear();
        m_tiles.resize(m_width * m_height);

        // Tiles
        uint32_t tileCount;
        file.read(reinterpret_cast<char*>(&tileCount), sizeof(tileCount));
        for (uint32_t i = 0; i < tileCount; ++i) {
            uint16_t tx, ty;
            uint8_t tt;
            file.read(reinterpret_cast<char*>(&tx), sizeof(tx));
            file.read(reinterpret_cast<char*>(&ty), sizeof(ty));
            file.read(reinterpret_cast<char*>(&tt), sizeof(tt));

            // Version 2: CollisionShape 읽기
            uint8_t ts = static_cast<uint8_t>(CollisionShape::Full);
            if (version == FILE_VERSION) {
                file.read(reinterpret_cast<char*>(&ts), sizeof(ts));
            } else {
                // Version 1: 타입에 따라 기본 CollisionShape 설정
                if (static_cast<TileType>(tt) == TileType::Platform) {
                    ts = static_cast<uint8_t>(CollisionShape::Platform);
                }
            }

            if (tx < static_cast<uint16_t>(m_width) && ty < static_cast<uint16_t>(m_height)) {
                // Y좌표 그대로 사용
                m_tiles[ty * m_width + tx].type = static_cast<TileType>(tt);
                m_tiles[ty * m_width + tx].shape = static_cast<CollisionShape>(ts);
            }
        }

        // Player Spawn (타일 좌표로 저장되어 있음 - 픽셀 좌표로 변환)
        int32_t spawnX, spawnY;
        file.read(reinterpret_cast<char*>(&spawnX), sizeof(spawnX));
        file.read(reinterpret_cast<char*>(&spawnY), sizeof(spawnY));
        // 타일 좌표를 픽셀 좌표로 변환 (Y 뒤집기: 에디터 하단 = 게임 하단)
        m_playerSpawnX = spawnX * TILE_SIZE;
        m_playerSpawnY = (m_height - 1 - spawnY) * TILE_SIZE;

        // Enemy Spawns
        m_enemySpawns.clear();
        uint32_t enemyCount;
        file.read(reinterpret_cast<char*>(&enemyCount), sizeof(enemyCount));
        for (uint32_t i = 0; i < enemyCount; ++i) {
            int32_t ex, ey;
            uint8_t et;
            file.read(reinterpret_cast<char*>(&ex), sizeof(ex));
            file.read(reinterpret_cast<char*>(&ey), sizeof(ey));
            file.read(reinterpret_cast<char*>(&et), sizeof(et));
            // 타일 좌표를 픽셀 좌표로 변환 (Y 뒤집기)
            m_enemySpawns.push_back({ex * TILE_SIZE, (m_height - 1 - ey) * TILE_SIZE, et});
        }

        return true;
    }

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
    std::vector<TileData> m_tiles;
    int m_playerSpawnX = -1;
    int m_playerSpawnY = -1;
    std::vector<std::tuple<int, int, uint8_t>> m_enemySpawns;
};
