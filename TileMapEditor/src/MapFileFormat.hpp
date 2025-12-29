#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>

// 바이너리 맵 파일 형식 (.tilemap)
//
// 파일 구조:
// [Header]
//   - Magic Number: 4 bytes ("TMAP")
//   - Version: 2 bytes (uint16_t)
//   - Grid Size: 2 bytes (uint16_t) - 타일 크기 (픽셀)
//   - Map Width: 4 bytes (uint32_t) - 가로 타일 개수
//   - Map Height: 4 bytes (uint32_t) - 세로 타일 개수
//   - Layer Count: 2 bytes (uint16_t)
// [Layers]
//   For each layer:
//     - Name Length: 1 byte (uint8_t)
//     - Name: N bytes (char[])
//     - Visible: 1 byte (bool)
//     - Tile Count: 4 bytes (uint32_t) - 비어있지 않은 타일 수
//     - Tiles: N * TileData
// [Spawns]
//   - Player Spawn: 8 bytes (int32_t x, int32_t y) - (-1,-1) if not set
//   - Enemy Spawn Count: 4 bytes (uint32_t)
//   - Enemy Spawns: N * EnemySpawnData

namespace MapFile {

constexpr char MAGIC[4] = {'T', 'M', 'A', 'P'};
constexpr uint16_t VERSION = 2;  // 버전 2: CollisionShape 추가
constexpr uint16_t VERSION_1 = 1;  // 이전 버전 호환용

enum class TileType : uint8_t {
    Empty = 0,
    Solid = 1,
    Platform = 2,
    // 확장 가능
};

// 충돌 형태 (경사면 및 특수 충돌)
enum class CollisionShape : uint8_t {
    None = 0,           // 충돌 없음 (Empty 타일용)
    Full = 1,           // 전체 타일 충돌 (기본 Solid)
    SlopeLeftUp = 2,    // 왼쪽 아래 → 오른쪽 위 (/)
    SlopeRightUp = 3,   // 오른쪽 아래 → 왼쪽 위 (\)
    HalfTop = 4,        // 상단 절반만 충돌
    HalfBottom = 5,     // 하단 절반만 충돌
    HalfLeft = 6,       // 좌측 절반만 충돌
    HalfRight = 7,      // 우측 절반만 충돌
    Platform = 8,       // 플랫폼 (위에서만 충돌)
};

// Version 1 타일 데이터 (이전 버전 호환용)
struct TileDataV1 {
    uint16_t x;          // 타일 x 좌표
    uint16_t y;          // 타일 y 좌표
    TileType type;       // 타일 타입
    uint8_t tilesetX;    // 타일셋 내 x 인덱스 (시각적 표현용, Phase 2)
    uint8_t tilesetY;    // 타일셋 내 y 인덱스 (시각적 표현용, Phase 2)
    uint8_t reserved;    // 예약 (정렬용)
};

// Version 2 타일 데이터 (충돌 형태 포함)
struct TileData {
    uint16_t x;              // 타일 x 좌표
    uint16_t y;              // 타일 y 좌표
    TileType type;           // 타일 타입
    CollisionShape shape;    // 충돌 형태
    uint8_t tilesetX;        // 타일셋 내 x 인덱스 (시각적 표현용, Phase 2)
    uint8_t tilesetY;        // 타일셋 내 y 인덱스 (시각적 표현용, Phase 2)
};

struct EnemySpawn {
    int32_t x;           // 월드 x 좌표 (픽셀)
    int32_t y;           // 월드 y 좌표 (픽셀)
    uint8_t enemyType;   // 적 타입 ID
    uint8_t reserved[3]; // 예약 (정렬용)
};

struct Layer {
    std::string name;
    bool visible;
    std::vector<TileData> tiles;
};

struct MapData {
    uint16_t gridSize;
    uint32_t width;
    uint32_t height;
    std::vector<Layer> layers;
    int32_t playerSpawnX;
    int32_t playerSpawnY;
    std::vector<EnemySpawn> enemySpawns;

    MapData() : gridSize(32), width(60), height(33),
                playerSpawnX(-1), playerSpawnY(-1) {}

    bool saveToFile(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        // Header
        file.write(MAGIC, 4);
        file.write(reinterpret_cast<const char*>(&VERSION), sizeof(VERSION));
        file.write(reinterpret_cast<const char*>(&gridSize), sizeof(gridSize));
        file.write(reinterpret_cast<const char*>(&width), sizeof(width));
        file.write(reinterpret_cast<const char*>(&height), sizeof(height));
        uint16_t layerCount = static_cast<uint16_t>(layers.size());
        file.write(reinterpret_cast<const char*>(&layerCount), sizeof(layerCount));

        // Layers
        for (const auto& layer : layers) {
            uint8_t nameLen = static_cast<uint8_t>(layer.name.size());
            file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
            file.write(layer.name.c_str(), nameLen);
            uint8_t visible = layer.visible ? 1 : 0;
            file.write(reinterpret_cast<const char*>(&visible), sizeof(visible));
            uint32_t tileCount = static_cast<uint32_t>(layer.tiles.size());
            file.write(reinterpret_cast<const char*>(&tileCount), sizeof(tileCount));
            for (const auto& tile : layer.tiles) {
                file.write(reinterpret_cast<const char*>(&tile), sizeof(TileData));
            }
        }

        // Spawns
        file.write(reinterpret_cast<const char*>(&playerSpawnX), sizeof(playerSpawnX));
        file.write(reinterpret_cast<const char*>(&playerSpawnY), sizeof(playerSpawnY));
        uint32_t enemyCount = static_cast<uint32_t>(enemySpawns.size());
        file.write(reinterpret_cast<const char*>(&enemyCount), sizeof(enemyCount));
        for (const auto& spawn : enemySpawns) {
            file.write(reinterpret_cast<const char*>(&spawn), sizeof(EnemySpawn));
        }

        return true;
    }

    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        // Header
        char magic[4];
        file.read(magic, 4);
        if (magic[0] != MAGIC[0] || magic[1] != MAGIC[1] ||
            magic[2] != MAGIC[2] || magic[3] != MAGIC[3]) {
            return false;
        }

        uint16_t version;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        if (version != VERSION && version != VERSION_1) return false;

        file.read(reinterpret_cast<char*>(&gridSize), sizeof(gridSize));
        file.read(reinterpret_cast<char*>(&width), sizeof(width));
        file.read(reinterpret_cast<char*>(&height), sizeof(height));
        uint16_t layerCount;
        file.read(reinterpret_cast<char*>(&layerCount), sizeof(layerCount));

        // Layers
        layers.clear();
        layers.resize(layerCount);
        for (auto& layer : layers) {
            uint8_t nameLen;
            file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
            layer.name.resize(nameLen);
            file.read(&layer.name[0], nameLen);
            uint8_t visible;
            file.read(reinterpret_cast<char*>(&visible), sizeof(visible));
            layer.visible = visible != 0;
            uint32_t tileCount;
            file.read(reinterpret_cast<char*>(&tileCount), sizeof(tileCount));
            layer.tiles.resize(tileCount);

            if (version == VERSION_1) {
                // Version 1: TileDataV1에서 읽고 TileData로 변환
                for (auto& tile : layer.tiles) {
                    TileDataV1 v1Tile;
                    file.read(reinterpret_cast<char*>(&v1Tile), sizeof(TileDataV1));
                    tile.x = v1Tile.x;
                    tile.y = v1Tile.y;
                    tile.type = v1Tile.type;
                    tile.tilesetX = v1Tile.tilesetX;
                    tile.tilesetY = v1Tile.tilesetY;
                    // Version 1에서는 CollisionShape이 없으므로 타입에 따라 기본값 설정
                    if (v1Tile.type == TileType::Empty) {
                        tile.shape = CollisionShape::None;
                    } else if (v1Tile.type == TileType::Platform) {
                        tile.shape = CollisionShape::Platform;
                    } else {
                        tile.shape = CollisionShape::Full;
                    }
                }
            } else {
                // Version 2: TileData 직접 읽기
                for (auto& tile : layer.tiles) {
                    file.read(reinterpret_cast<char*>(&tile), sizeof(TileData));
                }
            }
        }

        // Spawns
        file.read(reinterpret_cast<char*>(&playerSpawnX), sizeof(playerSpawnX));
        file.read(reinterpret_cast<char*>(&playerSpawnY), sizeof(playerSpawnY));
        uint32_t enemyCount;
        file.read(reinterpret_cast<char*>(&enemyCount), sizeof(enemyCount));
        enemySpawns.resize(enemyCount);
        for (auto& spawn : enemySpawns) {
            file.read(reinterpret_cast<char*>(&spawn), sizeof(EnemySpawn));
        }

        return true;
    }
};

} // namespace MapFile
