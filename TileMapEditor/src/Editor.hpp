#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>

// 타일 타입 (게임의 TileMap과 동일)
enum class TileType : uint8_t {
    Empty = 0,
    Solid = 1,
    Platform = 2
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

// 에디터 도구
enum class EditorTool {
    Brush,
    Eraser,
    PlayerSpawn,
    EnemySpawn,
    CollisionShape  // 충돌 형태 설정 도구
};

// 개별 타일 데이터
struct EditorTile {
    TileType type = TileType::Empty;
    CollisionShape shape = CollisionShape::None;
};

// 레이어 정보
struct EditorLayer {
    std::string name;
    bool visible = true;
    std::vector<std::vector<EditorTile>> tiles;

    EditorLayer(const std::string& layerName, int width, int height)
        : name(layerName)
    {
        tiles.resize(height, std::vector<EditorTile>(width));
    }

    void resize(int width, int height) {
        tiles.resize(height);
        for (auto& row : tiles) {
            row.resize(width);
        }
    }
};

class Editor {
public:
    Editor(unsigned int windowWidth, unsigned int windowHeight);

    void run();

private:
    // 이벤트 처리
    void handleEvents();
    void handleMouseClick(sf::Vector2i mousePos, bool isLeftButton);
    void handleMouseDrag(sf::Vector2i mousePos, bool isLeftButton);
    void handleKeyPress(sf::Keyboard::Key key);

    // 업데이트
    void update(float deltaTime);

    // 렌더링
    void render();
    void renderGrid();
    void renderTiles();
    void renderCollisionOverlay();  // 충돌 오버레이 렌더링
    void renderSpawns();
    void renderUI();
    void renderToolbar();
    void renderLayerPanel();
    void renderTileTypePanel();
    void renderCollisionShapePanel();  // 충돌 형태 선택 패널
    void renderMapSettingsPanel();

    // UI 상호작용
    bool isMouseOverUI(sf::Vector2i mousePos) const;
    sf::Vector2i screenToTile(sf::Vector2i screenPos) const;

    // 맵 조작
    void setTile(int x, int y, TileType type);
    void setTileShape(int x, int y, CollisionShape shape);
    TileType getTile(int x, int y) const;
    CollisionShape getTileShape(int x, int y) const;
    void resizeMap(int newWidth, int newHeight);

    // 레이어 조작
    void addLayer(const std::string& name);
    void removeLayer(int index);
    void selectLayer(int index);

    // 파일 조작
    void newMap();
    void saveMap(const std::string& filename);
    void loadMap(const std::string& filename);

    // 윈도우 및 뷰
    sf::RenderWindow m_window;
    sf::View m_mapView;      // 맵 캔버스 뷰
    sf::View m_uiView;       // UI 뷰 (고정)

    // 폰트
    sf::Font m_font;

    // 맵 설정
    int m_gridSize = 32;
    int m_mapWidth = 60;
    int m_mapHeight = 33;

    // 레이어
    std::vector<EditorLayer> m_layers;
    int m_currentLayerIndex = 0;

    // 현재 도구 및 타일 타입
    EditorTool m_currentTool = EditorTool::Brush;
    TileType m_currentTileType = TileType::Solid;
    CollisionShape m_currentCollisionShape = CollisionShape::Full;  // 현재 선택된 충돌 형태
    bool m_showCollisionOverlay = true;  // 충돌 오버레이 표시 여부

    // 스폰 위치
    sf::Vector2i m_playerSpawn = {-1, -1};
    std::vector<sf::Vector2i> m_enemySpawns;

    // 카메라
    sf::Vector2f m_cameraPos = {0.f, 0.f};
    float m_zoom = 1.f;
    sf::Vector2f m_defaultViewSize;  // 초기 뷰 크기 저장

    // 마우스 상태
    bool m_isDragging = false;
    sf::Vector2i m_lastMousePos;
    sf::Vector2i m_currentMousePos;  // 현재 마우스 위치 (화면 좌표)

    // UI 영역
    sf::FloatRect m_toolbarRect;
    sf::FloatRect m_layerPanelRect;
    sf::FloatRect m_tileTypePanelRect;
    sf::FloatRect m_collisionShapePanelRect;  // 충돌 형태 패널 영역
    sf::FloatRect m_mapSettingsRect;
    sf::FloatRect m_canvasRect;

    // 상태
    bool m_isRunning = true;
    std::string m_currentFilename;
    bool m_hasUnsavedChanges = false;
};
