#include "Editor.hpp"
#include <iostream>
#include <fstream>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <array>
#include <memory>

// macOS 파일 다이얼로그 (osascript 사용)
#ifdef __APPLE__
std::string openSaveFileDialog(const std::string& defaultName = "level.tilemap") {
    // choose file name은 file specifier를 반환하므로 as text로 변환 후 POSIX path 사용
    std::string command = "osascript -e 'set chosenFile to choose file name with prompt \"Save Tilemap As\" default name \"" + defaultName + "\"' "
                          "-e 'return POSIX path of chosenFile' 2>/dev/null";

    std::array<char, 1024> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (pipe) {
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
    }

    // 줄바꿈 제거
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    // .tilemap 확장자 자동 추가
    if (!result.empty() && result.find(".tilemap") == std::string::npos) {
        result += ".tilemap";
    }

    return result;
}

std::string openLoadFileDialog() {
    // choose file은 alias를 반환하므로 POSIX path로 변환
    std::string command = "osascript -e 'set chosenFile to choose file with prompt \"Open Tilemap\"' "
                          "-e 'return POSIX path of chosenFile' 2>/dev/null";

    std::array<char, 1024> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (pipe) {
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
    }

    // 줄바꿈 제거
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}
#else
// 다른 플랫폼용 기본 구현 (빈 문자열 반환 = 취소)
std::string openSaveFileDialog(const std::string& = "level.tilemap") {
    return "";
}
std::string openLoadFileDialog() {
    return "";
}
#endif

// 바이너리 파일 매직 넘버 및 버전
constexpr char FILE_MAGIC[4] = {'T', 'M', 'A', 'P'};
constexpr uint16_t FILE_VERSION = 2;  // Version 2: CollisionShape 추가
constexpr uint16_t FILE_VERSION_1 = 1;  // 이전 버전 호환용

Editor::Editor(unsigned int windowWidth, unsigned int windowHeight)
    : m_window(sf::VideoMode({windowWidth, windowHeight}), "TileMap Editor")
{
    m_window.setFramerateLimit(60);

    // 폰트 로드
    if (!m_font.openFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
        std::cerr << "Failed to load font!" << std::endl;
    }

    // UI 영역 설정
    float toolbarHeight = 40.f;
    float sidePanelWidth = 200.f;

    m_toolbarRect = {{0.f, 0.f}, {static_cast<float>(windowWidth), toolbarHeight}};
    m_layerPanelRect = {{static_cast<float>(windowWidth) - sidePanelWidth, toolbarHeight},
                        {sidePanelWidth, 150.f}};
    m_tileTypePanelRect = {{static_cast<float>(windowWidth) - sidePanelWidth, toolbarHeight + 150.f},
                           {sidePanelWidth, 120.f}};
    m_collisionShapePanelRect = {{static_cast<float>(windowWidth) - sidePanelWidth, toolbarHeight + 270.f},
                                  {sidePanelWidth, 240.f}};  // 충돌 형태 패널
    m_mapSettingsRect = {{0.f, toolbarHeight},
                         {sidePanelWidth, 200.f}};
    m_canvasRect = {{sidePanelWidth, toolbarHeight},
                    {static_cast<float>(windowWidth) - sidePanelWidth * 2, static_cast<float>(windowHeight) - toolbarHeight}};

    // 뷰 설정
    m_uiView = sf::View(sf::FloatRect({0.f, 0.f}, {static_cast<float>(windowWidth), static_cast<float>(windowHeight)}));

    // 맵 뷰: 캔버스 크기를 기본 뷰 크기로 저장
    m_defaultViewSize = {m_canvasRect.size.x, m_canvasRect.size.y};
    m_mapView = sf::View(sf::FloatRect({0.f, 0.f}, m_defaultViewSize));
    m_mapView.setViewport(sf::FloatRect(
        {m_canvasRect.position.x / windowWidth, m_canvasRect.position.y / windowHeight},
        {m_canvasRect.size.x / windowWidth, m_canvasRect.size.y / windowHeight}
    ));

    // 기본 레이어 추가
    addLayer("Ground");

    // 초기 카메라 위치를 맵 중앙으로 설정
    m_cameraPos = {
        static_cast<float>(m_mapWidth * m_gridSize) / 2.f,
        static_cast<float>(m_mapHeight * m_gridSize) / 2.f
    };
    m_mapView.setCenter(m_cameraPos);
}

void Editor::run() {
    sf::Clock clock;

    while (m_isRunning && m_window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        handleEvents();
        update(deltaTime);
        render();
    }
}

void Editor::handleEvents() {
    while (const auto event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            m_isRunning = false;
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            handleKeyPress(keyPressed->code);
        }

        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2i mousePos = {mousePressed->position.x, mousePressed->position.y};
            bool isLeft = mousePressed->button == sf::Mouse::Button::Left;
            handleMouseClick(mousePos, isLeft);
            m_isDragging = true;
            m_lastMousePos = mousePos;
        }

        if (event->is<sf::Event::MouseButtonReleased>()) {
            m_isDragging = false;
        }

        if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
            sf::Vector2i mousePos = {mouseMoved->position.x, mouseMoved->position.y};
            m_currentMousePos = mousePos;  // 항상 현재 마우스 위치 업데이트
            if (m_isDragging) {
                handleMouseDrag(mousePos, sf::Mouse::isButtonPressed(sf::Mouse::Button::Left));
                m_lastMousePos = mousePos;
            }
        }

        if (const auto* mouseScrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
            // 줌: delta 기반으로 직접 계산
            // delta > 0 (위로 스크롤) = 확대 (뷰 크기 감소)
            // delta < 0 (아래로 스크롤) = 축소 (뷰 크기 증가)
            float zoomAmount = 1.f - mouseScrolled->delta * 0.05f;
            m_zoom *= zoomAmount;
            m_zoom = std::max(0.25f, std::min(4.f, m_zoom));
            // 고정된 기본 뷰 크기를 기준으로 줌 적용
            m_mapView.setSize({m_defaultViewSize.x * m_zoom, m_defaultViewSize.y * m_zoom});
        }
    }
}

void Editor::handleMouseClick(sf::Vector2i mousePos, bool isLeftButton) {
    sf::Vector2f mousePosF = {static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)};

    // 캔버스 영역 클릭
    if (m_canvasRect.contains(mousePosF)) {
        sf::Vector2i tilePos = screenToTile(mousePos);

        if (tilePos.x >= 0 && tilePos.x < m_mapWidth &&
            tilePos.y >= 0 && tilePos.y < m_mapHeight) {

            if (isLeftButton) {
                switch (m_currentTool) {
                    case EditorTool::Brush:
                        setTile(tilePos.x, tilePos.y, m_currentTileType);
                        break;
                    case EditorTool::Eraser:
                        setTile(tilePos.x, tilePos.y, TileType::Empty);
                        // 해당 위치의 스폰도 삭제
                        if (m_playerSpawn == tilePos) {
                            m_playerSpawn = {-1, -1};
                        }
                        m_enemySpawns.erase(
                            std::remove(m_enemySpawns.begin(), m_enemySpawns.end(), tilePos),
                            m_enemySpawns.end()
                        );
                        break;
                    case EditorTool::PlayerSpawn:
                        m_playerSpawn = tilePos;
                        break;
                    case EditorTool::EnemySpawn:
                        m_enemySpawns.push_back(tilePos);
                        break;
                    case EditorTool::CollisionShape:
                        // 충돌 형태 설정 (타일이 있는 경우에만)
                        if (getTile(tilePos.x, tilePos.y) != TileType::Empty) {
                            setTileShape(tilePos.x, tilePos.y, m_currentCollisionShape);
                        }
                        break;
                }
                m_hasUnsavedChanges = true;
            } else {
                // 우클릭: 지우기 (타일 + 스폰)
                setTile(tilePos.x, tilePos.y, TileType::Empty);
                if (m_playerSpawn == tilePos) {
                    m_playerSpawn = {-1, -1};
                }
                m_enemySpawns.erase(
                    std::remove(m_enemySpawns.begin(), m_enemySpawns.end(), tilePos),
                    m_enemySpawns.end()
                );
                m_hasUnsavedChanges = true;
            }
        }
        return;
    }

    // 툴바 클릭
    if (m_toolbarRect.contains(mousePosF)) {
        float buttonWidth = 80.f;
        int buttonIndex = static_cast<int>(mousePos.x / buttonWidth);

        switch (buttonIndex) {
            case 0: newMap(); break;
            case 1: {
                // Save with file dialog
                std::string defaultName = m_currentFilename.empty() ? "level.tilemap" : m_currentFilename;
                // 경로에서 파일명만 추출
                size_t lastSlash = defaultName.find_last_of('/');
                if (lastSlash != std::string::npos) {
                    defaultName = defaultName.substr(lastSlash + 1);
                }
                std::string path = openSaveFileDialog(defaultName);
                if (!path.empty()) {
                    saveMap(path);
                }
                break;
            }
            case 2: {
                // Load with file dialog
                std::string path = openLoadFileDialog();
                if (!path.empty()) {
                    loadMap(path);
                }
                break;
            }
            case 3: m_currentTool = EditorTool::Brush; break;
            case 4: m_currentTool = EditorTool::Eraser; break;
            case 5: m_currentTool = EditorTool::PlayerSpawn; break;
            case 6: m_currentTool = EditorTool::EnemySpawn; break;
            case 7: m_currentTool = EditorTool::CollisionShape; break;
        }
        return;
    }

    // 타일 타입 패널 클릭
    if (m_tileTypePanelRect.contains(mousePosF)) {
        float relY = mousePos.y - m_tileTypePanelRect.position.y - 30.f;
        int typeIndex = static_cast<int>(relY / 30.f);
        if (typeIndex >= 0 && typeIndex <= 2) {
            m_currentTileType = static_cast<TileType>(typeIndex);
        }
        return;
    }

    // 레이어 패널 클릭
    if (m_layerPanelRect.contains(mousePosF)) {
        float relY = mousePos.y - m_layerPanelRect.position.y - 60.f;
        int layerIndex = static_cast<int>(relY / 25.f);
        if (layerIndex >= 0 && layerIndex < static_cast<int>(m_layers.size())) {
            selectLayer(layerIndex);
        }
        return;
    }

    // 충돌 형태 패널 클릭
    if (m_collisionShapePanelRect.contains(mousePosF)) {
        // 오버레이 토글 버튼 클릭 확인
        sf::FloatRect toggleRect = {
            {m_collisionShapePanelRect.position.x + 130.f, m_collisionShapePanelRect.position.y + 5.f},
            {60.f, 18.f}
        };
        if (toggleRect.contains(mousePosF)) {
            m_showCollisionOverlay = !m_showCollisionOverlay;
            return;
        }

        // 충돌 형태 선택
        float relY = mousePos.y - m_collisionShapePanelRect.position.y - 30.f;
        int shapeIndex = static_cast<int>(relY / 25.f);
        if (shapeIndex >= 0 && shapeIndex < 8) {
            m_currentCollisionShape = static_cast<CollisionShape>(shapeIndex + 1);  // None=0이므로 +1
            m_currentTool = EditorTool::CollisionShape;  // 자동으로 도구 전환
        }
        return;
    }
}

void Editor::handleMouseDrag(sf::Vector2i mousePos, bool isLeftButton) {
    sf::Vector2f mousePosF = {static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)};

    // 캔버스에서 드래그: 타일 연속 배치 (선형 보간으로 스킵 방지)
    if (m_canvasRect.contains(mousePosF)) {
        sf::Vector2i currentTile = screenToTile(mousePos);
        sf::Vector2i lastTile = screenToTile(m_lastMousePos);

        // Bresenham 선 알고리즘으로 두 타일 사이의 모든 타일에 적용
        int dx = std::abs(currentTile.x - lastTile.x);
        int dy = std::abs(currentTile.y - lastTile.y);
        int sx = lastTile.x < currentTile.x ? 1 : -1;
        int sy = lastTile.y < currentTile.y ? 1 : -1;
        int err = dx - dy;

        int x = lastTile.x;
        int y = lastTile.y;

        while (true) {
            // 현재 타일에 적용
            if (x >= 0 && x < m_mapWidth && y >= 0 && y < m_mapHeight) {
                if (isLeftButton) {
                    if (m_currentTool == EditorTool::Brush) {
                        setTile(x, y, m_currentTileType);
                        m_hasUnsavedChanges = true;
                    } else if (m_currentTool == EditorTool::Eraser) {
                        setTile(x, y, TileType::Empty);
                        // 스폰도 삭제
                        sf::Vector2i tilePos = {x, y};
                        if (m_playerSpawn == tilePos) {
                            m_playerSpawn = {-1, -1};
                        }
                        m_enemySpawns.erase(
                            std::remove(m_enemySpawns.begin(), m_enemySpawns.end(), tilePos),
                            m_enemySpawns.end()
                        );
                        m_hasUnsavedChanges = true;
                    } else if (m_currentTool == EditorTool::CollisionShape) {
                        // 충돌 형태 설정 (타일이 있는 경우에만)
                        if (getTile(x, y) != TileType::Empty) {
                            setTileShape(x, y, m_currentCollisionShape);
                            m_hasUnsavedChanges = true;
                        }
                    }
                } else {
                    // 우클릭: 지우기
                    setTile(x, y, TileType::Empty);
                    sf::Vector2i tilePos = {x, y};
                    if (m_playerSpawn == tilePos) {
                        m_playerSpawn = {-1, -1};
                    }
                    m_enemySpawns.erase(
                        std::remove(m_enemySpawns.begin(), m_enemySpawns.end(), tilePos),
                        m_enemySpawns.end()
                    );
                    m_hasUnsavedChanges = true;
                }
            }

            if (x == currentTile.x && y == currentTile.y) break;

            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x += sx;
            }
            if (e2 < dx) {
                err += dx;
                y += sy;
            }
        }
    }

    // 미들 버튼 드래그: 카메라 이동
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle)) {
        sf::Vector2f delta = {
            static_cast<float>(m_lastMousePos.x - mousePos.x) * m_zoom,
            static_cast<float>(m_lastMousePos.y - mousePos.y) * m_zoom
        };
        m_cameraPos += delta;
        m_mapView.setCenter(m_cameraPos);
    }
}

void Editor::handleKeyPress(sf::Keyboard::Key key) {
    switch (key) {
        case sf::Keyboard::Key::Num1:
            m_currentTool = EditorTool::Brush;
            break;
        case sf::Keyboard::Key::Num2:
            m_currentTool = EditorTool::Eraser;
            break;
        case sf::Keyboard::Key::Num3:
            m_currentTool = EditorTool::PlayerSpawn;
            break;
        case sf::Keyboard::Key::Num4:
            m_currentTool = EditorTool::EnemySpawn;
            break;
        case sf::Keyboard::Key::Num5:
            m_currentTool = EditorTool::CollisionShape;
            break;
        case sf::Keyboard::Key::C:
            // C 키: 충돌 오버레이 토글 (Ctrl/Cmd 없을 때)
            if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) &&
                !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LSystem)) {
                m_showCollisionOverlay = !m_showCollisionOverlay;
            }
            break;
        case sf::Keyboard::Key::S:
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LSystem)) {
                // Ctrl/Cmd + S: Save with file dialog
                std::string defaultName = m_currentFilename.empty() ? "level.tilemap" : m_currentFilename;
                size_t lastSlash = defaultName.find_last_of('/');
                if (lastSlash != std::string::npos) {
                    defaultName = defaultName.substr(lastSlash + 1);
                }
                std::string path = openSaveFileDialog(defaultName);
                if (!path.empty()) {
                    saveMap(path);
                }
            }
            break;
        case sf::Keyboard::Key::O:
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LSystem)) {
                // Ctrl/Cmd + O: Load with file dialog
                std::string path = openLoadFileDialog();
                if (!path.empty()) {
                    loadMap(path);
                }
            }
            break;
        case sf::Keyboard::Key::N:
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LSystem)) {
                newMap();
            }
            break;
        case sf::Keyboard::Key::Num0:
        case sf::Keyboard::Key::Home:
            // 줌 리셋 (100%로)
            m_zoom = 1.f;
            m_mapView.setSize(m_defaultViewSize);
            break;
        default:
            break;
    }
}

void Editor::update(float deltaTime) {
    // 화살표 키 또는 WASD로 카메라 이동 (확대 시 스크롤)
    float scrollSpeed = 500.f * m_zoom;  // 줌 레벨에 따라 속도 조정
    sf::Vector2f movement = {0.f, 0.f};

    // 화살표 키
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        movement.x -= scrollSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        movement.x += scrollSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
        movement.y -= scrollSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
        movement.y += scrollSpeed * deltaTime;
    }

    // WASD 키 (Ctrl/Cmd가 눌려있지 않을 때만)
    bool ctrlOrCmd = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LSystem);
    if (!ctrlOrCmd) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
            movement.x -= scrollSpeed * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
            movement.x += scrollSpeed * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
            movement.y -= scrollSpeed * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
            movement.y += scrollSpeed * deltaTime;
        }
    }

    // 카메라 위치 업데이트
    if (movement.x != 0.f || movement.y != 0.f) {
        m_cameraPos += movement;

        // 카메라 범위 제한 (맵 바깥으로 너무 많이 나가지 않도록)
        float mapPixelWidth = static_cast<float>(m_mapWidth * m_gridSize);
        float mapPixelHeight = static_cast<float>(m_mapHeight * m_gridSize);
        float margin = 200.f * m_zoom;  // 여유 공간

        m_cameraPos.x = std::max(-margin, std::min(mapPixelWidth + margin, m_cameraPos.x));
        m_cameraPos.y = std::max(-margin, std::min(mapPixelHeight + margin, m_cameraPos.y));

        m_mapView.setCenter(m_cameraPos);
    }
}

void Editor::render() {
    m_window.clear(sf::Color{40, 40, 40});

    // 맵 캔버스 렌더링
    m_window.setView(m_mapView);

    // 타일맵 범위 배경 (편집 가능 영역 표시)
    sf::RectangleShape mapBounds;
    mapBounds.setPosition({0.f, 0.f});
    mapBounds.setSize({static_cast<float>(m_mapWidth * m_gridSize),
                       static_cast<float>(m_mapHeight * m_gridSize)});
    mapBounds.setFillColor(sf::Color{50, 50, 55});
    mapBounds.setOutlineThickness(3.f);
    mapBounds.setOutlineColor(sf::Color{100, 150, 200});
    m_window.draw(mapBounds);

    renderGrid();
    renderTiles();
    renderSpawns();

    // UI 렌더링
    m_window.setView(m_uiView);
    renderUI();

    // 캔버스 영역 테두리 (화면 좌표)
    sf::RectangleShape canvasBorder;
    canvasBorder.setPosition(m_canvasRect.position);
    canvasBorder.setSize(m_canvasRect.size);
    canvasBorder.setFillColor(sf::Color::Transparent);
    canvasBorder.setOutlineThickness(2.f);
    canvasBorder.setOutlineColor(sf::Color{80, 80, 80});
    m_window.draw(canvasBorder);

    m_window.display();
}

void Editor::renderGrid() {
    sf::RectangleShape line;
    line.setFillColor(sf::Color{60, 60, 60});

    // 줌에 따라 선 두께 조정 (축소시 더 두꺼운 선)
    float lineThickness = std::max(1.f, m_zoom * 1.5f);

    // 세로선
    for (int x = 0; x <= m_mapWidth; ++x) {
        line.setSize({lineThickness, static_cast<float>(m_mapHeight * m_gridSize)});
        line.setPosition({static_cast<float>(x * m_gridSize), 0.f});
        m_window.draw(line);
    }

    // 가로선
    for (int y = 0; y <= m_mapHeight; ++y) {
        line.setSize({static_cast<float>(m_mapWidth * m_gridSize), lineThickness});
        line.setPosition({0.f, static_cast<float>(y * m_gridSize)});
        m_window.draw(line);
    }
}

void Editor::renderTiles() {
    sf::RectangleShape tileShape;
    tileShape.setSize({static_cast<float>(m_gridSize - 1), static_cast<float>(m_gridSize - 1)});

    // 모든 보이는 레이어의 타일 렌더링
    for (const auto& layer : m_layers) {
        if (!layer.visible) continue;

        for (int y = 0; y < m_mapHeight && y < static_cast<int>(layer.tiles.size()); ++y) {
            for (int x = 0; x < m_mapWidth && x < static_cast<int>(layer.tiles[y].size()); ++x) {
                const EditorTile& tile = layer.tiles[y][x];
                if (tile.type == TileType::Empty) continue;

                tileShape.setPosition({static_cast<float>(x * m_gridSize + 1), static_cast<float>(y * m_gridSize + 1)});

                switch (tile.type) {
                    case TileType::Solid:
                        tileShape.setFillColor(sf::Color{80, 60, 40});
                        break;
                    case TileType::Platform:
                        tileShape.setFillColor(sf::Color{60, 100, 60});
                        break;
                    default:
                        continue;
                }

                m_window.draw(tileShape);
            }
        }
    }

    // 충돌 오버레이 렌더링
    if (m_showCollisionOverlay) {
        renderCollisionOverlay();
    }
}

void Editor::renderSpawns() {
    // 플레이어 스폰
    if (m_playerSpawn.x >= 0 && m_playerSpawn.y >= 0) {
        sf::CircleShape marker(m_gridSize / 3.f);
        marker.setFillColor(sf::Color{100, 200, 100});
        marker.setPosition({
            static_cast<float>(m_playerSpawn.x * m_gridSize + m_gridSize / 6.f),
            static_cast<float>(m_playerSpawn.y * m_gridSize + m_gridSize / 6.f)
        });
        m_window.draw(marker);

        sf::Text text(m_font, "P", 16);
        text.setFillColor(sf::Color::White);
        text.setPosition({
            static_cast<float>(m_playerSpawn.x * m_gridSize + m_gridSize / 3.f),
            static_cast<float>(m_playerSpawn.y * m_gridSize + m_gridSize / 6.f)
        });
        m_window.draw(text);
    }

    // 적 스폰
    for (const auto& spawn : m_enemySpawns) {
        sf::CircleShape marker(m_gridSize / 3.f);
        marker.setFillColor(sf::Color{200, 100, 100});
        marker.setPosition({
            static_cast<float>(spawn.x * m_gridSize + m_gridSize / 6.f),
            static_cast<float>(spawn.y * m_gridSize + m_gridSize / 6.f)
        });
        m_window.draw(marker);

        sf::Text text(m_font, "E", 16);
        text.setFillColor(sf::Color::White);
        text.setPosition({
            static_cast<float>(spawn.x * m_gridSize + m_gridSize / 3.f),
            static_cast<float>(spawn.y * m_gridSize + m_gridSize / 6.f)
        });
        m_window.draw(text);
    }
}

void Editor::renderCollisionOverlay() {
    // 충돌 형태별 색상 (반투명)
    auto getShapeColor = [](CollisionShape shape) -> sf::Color {
        switch (shape) {
            case CollisionShape::None:     return sf::Color::Transparent;
            case CollisionShape::Full:     return sf::Color{255, 0, 0, 80};      // 빨강
            case CollisionShape::SlopeLeftUp:  return sf::Color{0, 255, 255, 100};  // 시안
            case CollisionShape::SlopeRightUp: return sf::Color{255, 255, 0, 100};  // 노랑
            case CollisionShape::HalfTop:      return sf::Color{255, 128, 0, 80};   // 주황
            case CollisionShape::HalfBottom:   return sf::Color{128, 0, 255, 80};   // 보라
            case CollisionShape::HalfLeft:     return sf::Color{0, 128, 255, 80};   // 파랑
            case CollisionShape::HalfRight:    return sf::Color{255, 0, 128, 80};   // 분홍
            case CollisionShape::Platform:     return sf::Color{0, 255, 0, 100};    // 초록
            default: return sf::Color::Transparent;
        }
    };

    float size = static_cast<float>(m_gridSize);

    for (const auto& layer : m_layers) {
        if (!layer.visible) continue;

        for (int y = 0; y < m_mapHeight && y < static_cast<int>(layer.tiles.size()); ++y) {
            for (int x = 0; x < m_mapWidth && x < static_cast<int>(layer.tiles[y].size()); ++x) {
                const EditorTile& tile = layer.tiles[y][x];
                if (tile.type == TileType::Empty || tile.shape == CollisionShape::None) continue;

                float px = static_cast<float>(x * m_gridSize);
                float py = static_cast<float>(y * m_gridSize);
                sf::Color color = getShapeColor(tile.shape);

                switch (tile.shape) {
                    case CollisionShape::Full: {
                        sf::RectangleShape rect;
                        rect.setPosition({px, py});
                        rect.setSize({size, size});
                        rect.setFillColor(color);
                        m_window.draw(rect);
                        break;
                    }
                    case CollisionShape::SlopeLeftUp: {
                        // 왼쪽 아래 → 오른쪽 위 (/)
                        sf::ConvexShape triangle;
                        triangle.setPointCount(3);
                        triangle.setPoint(0, {px, py + size});          // 왼쪽 아래
                        triangle.setPoint(1, {px + size, py});          // 오른쪽 위
                        triangle.setPoint(2, {px + size, py + size});   // 오른쪽 아래
                        triangle.setFillColor(color);
                        m_window.draw(triangle);
                        break;
                    }
                    case CollisionShape::SlopeRightUp: {
                        // 오른쪽 아래 → 왼쪽 위 (\)
                        sf::ConvexShape triangle;
                        triangle.setPointCount(3);
                        triangle.setPoint(0, {px, py});                 // 왼쪽 위
                        triangle.setPoint(1, {px + size, py + size});   // 오른쪽 아래
                        triangle.setPoint(2, {px, py + size});          // 왼쪽 아래
                        triangle.setFillColor(color);
                        m_window.draw(triangle);
                        break;
                    }
                    case CollisionShape::HalfTop: {
                        sf::RectangleShape rect;
                        rect.setPosition({px, py});
                        rect.setSize({size, size / 2.f});
                        rect.setFillColor(color);
                        m_window.draw(rect);
                        break;
                    }
                    case CollisionShape::HalfBottom: {
                        sf::RectangleShape rect;
                        rect.setPosition({px, py + size / 2.f});
                        rect.setSize({size, size / 2.f});
                        rect.setFillColor(color);
                        m_window.draw(rect);
                        break;
                    }
                    case CollisionShape::HalfLeft: {
                        sf::RectangleShape rect;
                        rect.setPosition({px, py});
                        rect.setSize({size / 2.f, size});
                        rect.setFillColor(color);
                        m_window.draw(rect);
                        break;
                    }
                    case CollisionShape::HalfRight: {
                        sf::RectangleShape rect;
                        rect.setPosition({px + size / 2.f, py});
                        rect.setSize({size / 2.f, size});
                        rect.setFillColor(color);
                        m_window.draw(rect);
                        break;
                    }
                    case CollisionShape::Platform: {
                        // 상단에 얇은 선으로 표시
                        sf::RectangleShape rect;
                        rect.setPosition({px, py});
                        rect.setSize({size, size / 4.f});
                        rect.setFillColor(color);
                        m_window.draw(rect);
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
}

void Editor::renderUI() {
    renderToolbar();
    renderMapSettingsPanel();
    renderLayerPanel();
    renderTileTypePanel();
    renderCollisionShapePanel();
}

void Editor::renderToolbar() {
    // 툴바 배경
    sf::RectangleShape bg;
    bg.setPosition(m_toolbarRect.position);
    bg.setSize(m_toolbarRect.size);
    bg.setFillColor(sf::Color{50, 50, 50});
    m_window.draw(bg);

    // 버튼들
    std::vector<std::string> buttons = {"New", "Save", "Load", "Brush", "Eraser", "Player", "Enemy", "Collide"};
    float buttonWidth = 80.f;

    for (size_t i = 0; i < buttons.size(); ++i) {
        sf::RectangleShape btn;
        btn.setPosition({i * buttonWidth + 5.f, 5.f});
        btn.setSize({buttonWidth - 10.f, 30.f});

        // 현재 선택된 도구 강조
        bool isSelected = false;
        if (i == 3 && m_currentTool == EditorTool::Brush) isSelected = true;
        if (i == 4 && m_currentTool == EditorTool::Eraser) isSelected = true;
        if (i == 5 && m_currentTool == EditorTool::PlayerSpawn) isSelected = true;
        if (i == 6 && m_currentTool == EditorTool::EnemySpawn) isSelected = true;
        if (i == 7 && m_currentTool == EditorTool::CollisionShape) isSelected = true;

        btn.setFillColor(isSelected ? sf::Color{80, 120, 80} : sf::Color{70, 70, 70});
        btn.setOutlineThickness(1.f);
        btn.setOutlineColor(sf::Color{100, 100, 100});
        m_window.draw(btn);

        sf::Text text(m_font, buttons[i], 14);
        text.setFillColor(sf::Color::White);
        text.setPosition({i * buttonWidth + 15.f, 12.f});
        m_window.draw(text);
    }
}

void Editor::renderMapSettingsPanel() {
    // 패널 배경
    sf::RectangleShape bg;
    bg.setPosition(m_mapSettingsRect.position);
    bg.setSize(m_mapSettingsRect.size);
    bg.setFillColor(sf::Color{45, 45, 45});
    m_window.draw(bg);

    // 제목
    sf::Text title(m_font, "Map Settings", 14);
    title.setFillColor(sf::Color::White);
    title.setPosition({m_mapSettingsRect.position.x + 10.f, m_mapSettingsRect.position.y + 5.f});
    m_window.draw(title);

    // 설정 표시
    float y = m_mapSettingsRect.position.y + 30.f;

    sf::Text gridText(m_font, "Grid: " + std::to_string(m_gridSize) + "px", 12);
    gridText.setFillColor(sf::Color{200, 200, 200});
    gridText.setPosition({m_mapSettingsRect.position.x + 10.f, y});
    m_window.draw(gridText);

    sf::Text widthText(m_font, "Width: " + std::to_string(m_mapWidth), 12);
    widthText.setFillColor(sf::Color{200, 200, 200});
    widthText.setPosition({m_mapSettingsRect.position.x + 10.f, y + 20.f});
    m_window.draw(widthText);

    sf::Text heightText(m_font, "Height: " + std::to_string(m_mapHeight), 12);
    heightText.setFillColor(sf::Color{200, 200, 200});
    heightText.setPosition({m_mapSettingsRect.position.x + 10.f, y + 40.f});
    m_window.draw(heightText);

    sf::Text zoomText(m_font, "Zoom: " + std::to_string(static_cast<int>(100.f / m_zoom)) + "%", 12);
    zoomText.setFillColor(sf::Color{200, 200, 200});
    zoomText.setPosition({m_mapSettingsRect.position.x + 10.f, y + 60.f});
    m_window.draw(zoomText);

    // 마우스 위치 정보 (캔버스 내에서만 표시)
    sf::Vector2f mousePosF = {static_cast<float>(m_currentMousePos.x), static_cast<float>(m_currentMousePos.y)};
    if (m_canvasRect.contains(mousePosF)) {
        sf::Vector2i tilePos = screenToTile(m_currentMousePos);
        int pixelX = tilePos.x * m_gridSize;
        int pixelY = tilePos.y * m_gridSize;

        // 구분선
        sf::RectangleShape separator;
        separator.setPosition({m_mapSettingsRect.position.x + 10.f, y + 85.f});
        separator.setSize({m_mapSettingsRect.size.x - 20.f, 1.f});
        separator.setFillColor(sf::Color{80, 80, 80});
        m_window.draw(separator);

        // 마우스 좌표 제목
        sf::Text mouseTitle(m_font, "Mouse Position", 12);
        mouseTitle.setFillColor(sf::Color{150, 200, 255});
        mouseTitle.setPosition({m_mapSettingsRect.position.x + 10.f, y + 92.f});
        m_window.draw(mouseTitle);

        // 타일 좌표
        sf::Text tileText(m_font, "Tile: (" + std::to_string(tilePos.x) + ", " + std::to_string(tilePos.y) + ")", 12);
        tileText.setFillColor(sf::Color{200, 200, 200});
        tileText.setPosition({m_mapSettingsRect.position.x + 10.f, y + 112.f});
        m_window.draw(tileText);

        // 픽셀 좌표 (게임에서 사용하는 좌표)
        sf::Text pixelText(m_font, "Pixel: (" + std::to_string(pixelX) + ", " + std::to_string(pixelY) + ")", 12);
        pixelText.setFillColor(sf::Color{255, 200, 150});
        pixelText.setPosition({m_mapSettingsRect.position.x + 10.f, y + 132.f});
        m_window.draw(pixelText);
    }
}

void Editor::renderLayerPanel() {
    // 패널 배경
    sf::RectangleShape bg;
    bg.setPosition(m_layerPanelRect.position);
    bg.setSize(m_layerPanelRect.size);
    bg.setFillColor(sf::Color{45, 45, 45});
    m_window.draw(bg);

    // 제목
    sf::Text title(m_font, "Layers", 14);
    title.setFillColor(sf::Color::White);
    title.setPosition({m_layerPanelRect.position.x + 10.f, m_layerPanelRect.position.y + 5.f});
    m_window.draw(title);

    // + 버튼
    sf::RectangleShape addBtn;
    addBtn.setPosition({m_layerPanelRect.position.x + 10.f, m_layerPanelRect.position.y + 30.f});
    addBtn.setSize({25.f, 25.f});
    addBtn.setFillColor(sf::Color{70, 70, 70});
    m_window.draw(addBtn);

    sf::Text addText(m_font, "+", 16);
    addText.setFillColor(sf::Color::White);
    addText.setPosition({m_layerPanelRect.position.x + 17.f, m_layerPanelRect.position.y + 32.f});
    m_window.draw(addText);

    // 레이어 목록
    float y = m_layerPanelRect.position.y + 60.f;
    for (size_t i = 0; i < m_layers.size(); ++i) {
        sf::RectangleShape layerBg;
        layerBg.setPosition({m_layerPanelRect.position.x + 5.f, y});
        layerBg.setSize({m_layerPanelRect.size.x - 10.f, 22.f});
        layerBg.setFillColor(i == static_cast<size_t>(m_currentLayerIndex)
                            ? sf::Color{80, 80, 120} : sf::Color{60, 60, 60});
        m_window.draw(layerBg);

        // 가시성 토글
        sf::CircleShape visToggle(6.f);
        visToggle.setPosition({m_layerPanelRect.position.x + 10.f, y + 5.f});
        visToggle.setFillColor(m_layers[i].visible ? sf::Color::Green : sf::Color::Red);
        m_window.draw(visToggle);

        // 레이어 이름
        sf::Text layerName(m_font, m_layers[i].name, 12);
        layerName.setFillColor(sf::Color::White);
        layerName.setPosition({m_layerPanelRect.position.x + 30.f, y + 3.f});
        m_window.draw(layerName);

        y += 25.f;
    }
}

void Editor::renderTileTypePanel() {
    // 패널 배경
    sf::RectangleShape bg;
    bg.setPosition(m_tileTypePanelRect.position);
    bg.setSize(m_tileTypePanelRect.size);
    bg.setFillColor(sf::Color{45, 45, 45});
    m_window.draw(bg);

    // 제목
    sf::Text title(m_font, "Tile Type", 14);
    title.setFillColor(sf::Color::White);
    title.setPosition({m_tileTypePanelRect.position.x + 10.f, m_tileTypePanelRect.position.y + 5.f});
    m_window.draw(title);

    // 타일 타입 목록
    std::vector<std::pair<std::string, sf::Color>> types = {
        {"Empty", sf::Color{40, 40, 40}},
        {"Solid", sf::Color{80, 60, 40}},
        {"Platform", sf::Color{60, 100, 60}}
    };

    float y = m_tileTypePanelRect.position.y + 30.f;
    for (size_t i = 0; i < types.size(); ++i) {
        // 선택 표시
        sf::RectangleShape typeBg;
        typeBg.setPosition({m_tileTypePanelRect.position.x + 5.f, y});
        typeBg.setSize({m_tileTypePanelRect.size.x - 10.f, 25.f});
        typeBg.setFillColor(static_cast<size_t>(m_currentTileType) == i
                          ? sf::Color{80, 80, 120} : sf::Color{55, 55, 55});
        m_window.draw(typeBg);

        // 색상 미리보기
        sf::RectangleShape preview;
        preview.setPosition({m_tileTypePanelRect.position.x + 10.f, y + 3.f});
        preview.setSize({20.f, 20.f});
        preview.setFillColor(types[i].second);
        preview.setOutlineThickness(1.f);
        preview.setOutlineColor(sf::Color{100, 100, 100});
        m_window.draw(preview);

        // 이름
        sf::Text typeName(m_font, types[i].first, 12);
        typeName.setFillColor(sf::Color::White);
        typeName.setPosition({m_tileTypePanelRect.position.x + 40.f, y + 5.f});
        m_window.draw(typeName);

        y += 30.f;
    }
}

void Editor::renderCollisionShapePanel() {
    // 패널 배경
    sf::RectangleShape bg;
    bg.setPosition(m_collisionShapePanelRect.position);
    bg.setSize(m_collisionShapePanelRect.size);
    bg.setFillColor(sf::Color{45, 45, 45});
    m_window.draw(bg);

    // 제목
    sf::Text title(m_font, "Collision Shape", 14);
    title.setFillColor(sf::Color::White);
    title.setPosition({m_collisionShapePanelRect.position.x + 10.f, m_collisionShapePanelRect.position.y + 5.f});
    m_window.draw(title);

    // 오버레이 토글 버튼
    sf::RectangleShape toggleBtn;
    toggleBtn.setPosition({m_collisionShapePanelRect.position.x + 130.f, m_collisionShapePanelRect.position.y + 5.f});
    toggleBtn.setSize({60.f, 18.f});
    toggleBtn.setFillColor(m_showCollisionOverlay ? sf::Color{80, 120, 80} : sf::Color{70, 70, 70});
    m_window.draw(toggleBtn);

    sf::Text toggleText(m_font, m_showCollisionOverlay ? "ON" : "OFF", 10);
    toggleText.setFillColor(sf::Color::White);
    toggleText.setPosition({m_collisionShapePanelRect.position.x + 150.f, m_collisionShapePanelRect.position.y + 7.f});
    m_window.draw(toggleText);

    // 충돌 형태 목록
    std::vector<std::pair<std::string, sf::Color>> shapes = {
        {"Full",       sf::Color{255, 0, 0, 180}},
        {"Slope /",    sf::Color{0, 255, 255, 180}},
        {"Slope \\",   sf::Color{255, 255, 0, 180}},
        {"Half Top",   sf::Color{255, 128, 0, 180}},
        {"Half Btm",   sf::Color{128, 0, 255, 180}},
        {"Half L",     sf::Color{0, 128, 255, 180}},
        {"Half R",     sf::Color{255, 0, 128, 180}},
        {"Platform",   sf::Color{0, 255, 0, 180}}
    };

    // CollisionShape enum 순서: Full=1, SlopeLeftUp=2, SlopeRightUp=3, HalfTop=4, ...
    float y = m_collisionShapePanelRect.position.y + 30.f;
    for (size_t i = 0; i < shapes.size(); ++i) {
        CollisionShape shapeEnum = static_cast<CollisionShape>(i + 1);  // None=0이므로 +1

        // 선택 표시
        sf::RectangleShape shapeBg;
        shapeBg.setPosition({m_collisionShapePanelRect.position.x + 5.f, y});
        shapeBg.setSize({m_collisionShapePanelRect.size.x - 10.f, 22.f});
        shapeBg.setFillColor(m_currentCollisionShape == shapeEnum
                           ? sf::Color{80, 80, 120} : sf::Color{55, 55, 55});
        m_window.draw(shapeBg);

        // 색상 미리보기
        sf::RectangleShape preview;
        preview.setPosition({m_collisionShapePanelRect.position.x + 10.f, y + 2.f});
        preview.setSize({18.f, 18.f});
        preview.setFillColor(shapes[i].second);
        preview.setOutlineThickness(1.f);
        preview.setOutlineColor(sf::Color{100, 100, 100});
        m_window.draw(preview);

        // 이름
        sf::Text shapeName(m_font, shapes[i].first, 11);
        shapeName.setFillColor(sf::Color::White);
        shapeName.setPosition({m_collisionShapePanelRect.position.x + 35.f, y + 4.f});
        m_window.draw(shapeName);

        y += 25.f;
    }
}

bool Editor::isMouseOverUI(sf::Vector2i mousePos) const {
    sf::Vector2f pos = {static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)};
    return m_toolbarRect.contains(pos) ||
           m_layerPanelRect.contains(pos) ||
           m_tileTypePanelRect.contains(pos) ||
           m_collisionShapePanelRect.contains(pos) ||
           m_mapSettingsRect.contains(pos);
}

sf::Vector2i Editor::screenToTile(sf::Vector2i screenPos) const {
    // 스크린 좌표를 월드 좌표로 변환
    sf::Vector2f worldPos = m_window.mapPixelToCoords(screenPos, m_mapView);

    return {
        static_cast<int>(worldPos.x / m_gridSize),
        static_cast<int>(worldPos.y / m_gridSize)
    };
}

void Editor::setTile(int x, int y, TileType type) {
    if (m_currentLayerIndex < 0 || m_currentLayerIndex >= static_cast<int>(m_layers.size())) return;
    auto& layer = m_layers[m_currentLayerIndex];

    if (y >= 0 && y < static_cast<int>(layer.tiles.size()) &&
        x >= 0 && x < static_cast<int>(layer.tiles[y].size())) {
        layer.tiles[y][x].type = type;
        // 타일 타입에 따라 기본 충돌 형태 설정
        if (type == TileType::Empty) {
            layer.tiles[y][x].shape = CollisionShape::None;
        } else if (type == TileType::Platform) {
            layer.tiles[y][x].shape = CollisionShape::Platform;
        } else if (type == TileType::Solid) {
            // Solid 타일은 기존 shape이 None이면 Full로 설정
            if (layer.tiles[y][x].shape == CollisionShape::None ||
                layer.tiles[y][x].shape == CollisionShape::Platform) {
                layer.tiles[y][x].shape = CollisionShape::Full;
            }
        }
    }
}

void Editor::setTileShape(int x, int y, CollisionShape shape) {
    if (m_currentLayerIndex < 0 || m_currentLayerIndex >= static_cast<int>(m_layers.size())) return;
    auto& layer = m_layers[m_currentLayerIndex];

    if (y >= 0 && y < static_cast<int>(layer.tiles.size()) &&
        x >= 0 && x < static_cast<int>(layer.tiles[y].size())) {
        // 빈 타일이 아닌 경우에만 충돌 형태 설정
        if (layer.tiles[y][x].type != TileType::Empty) {
            layer.tiles[y][x].shape = shape;
        }
    }
}

TileType Editor::getTile(int x, int y) const {
    if (m_currentLayerIndex < 0 || m_currentLayerIndex >= static_cast<int>(m_layers.size()))
        return TileType::Empty;

    const auto& layer = m_layers[m_currentLayerIndex];

    if (y >= 0 && y < static_cast<int>(layer.tiles.size()) &&
        x >= 0 && x < static_cast<int>(layer.tiles[y].size())) {
        return layer.tiles[y][x].type;
    }
    return TileType::Empty;
}

CollisionShape Editor::getTileShape(int x, int y) const {
    if (m_currentLayerIndex < 0 || m_currentLayerIndex >= static_cast<int>(m_layers.size()))
        return CollisionShape::None;

    const auto& layer = m_layers[m_currentLayerIndex];

    if (y >= 0 && y < static_cast<int>(layer.tiles.size()) &&
        x >= 0 && x < static_cast<int>(layer.tiles[y].size())) {
        return layer.tiles[y][x].shape;
    }
    return CollisionShape::None;
}

void Editor::resizeMap(int newWidth, int newHeight) {
    m_mapWidth = newWidth;
    m_mapHeight = newHeight;

    for (auto& layer : m_layers) {
        layer.resize(newWidth, newHeight);
    }
}

void Editor::addLayer(const std::string& name) {
    m_layers.emplace_back(name, m_mapWidth, m_mapHeight);
    m_currentLayerIndex = static_cast<int>(m_layers.size()) - 1;
}

void Editor::removeLayer(int index) {
    if (index >= 0 && index < static_cast<int>(m_layers.size()) && m_layers.size() > 1) {
        m_layers.erase(m_layers.begin() + index);
        if (m_currentLayerIndex >= static_cast<int>(m_layers.size())) {
            m_currentLayerIndex = static_cast<int>(m_layers.size()) - 1;
        }
    }
}

void Editor::selectLayer(int index) {
    if (index >= 0 && index < static_cast<int>(m_layers.size())) {
        m_currentLayerIndex = index;
    }
}

void Editor::newMap() {
    m_layers.clear();
    addLayer("Ground");
    m_playerSpawn = {-1, -1};
    m_enemySpawns.clear();
    // 맵의 중앙으로 카메라 설정
    m_cameraPos = {
        static_cast<float>(m_mapWidth * m_gridSize) / 2.f,
        static_cast<float>(m_mapHeight * m_gridSize) / 2.f
    };
    m_mapView.setCenter(m_cameraPos);
    m_hasUnsavedChanges = false;
    m_currentFilename.clear();
}

void Editor::saveMap(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to save: " << filename << std::endl;
        return;
    }

    // Header
    file.write(FILE_MAGIC, 4);
    file.write(reinterpret_cast<const char*>(&FILE_VERSION), sizeof(FILE_VERSION));
    uint16_t gridSize = static_cast<uint16_t>(m_gridSize);
    file.write(reinterpret_cast<const char*>(&gridSize), sizeof(gridSize));
    uint32_t width = static_cast<uint32_t>(m_mapWidth);
    uint32_t height = static_cast<uint32_t>(m_mapHeight);
    file.write(reinterpret_cast<const char*>(&width), sizeof(width));
    file.write(reinterpret_cast<const char*>(&height), sizeof(height));

    // 모든 레이어의 타일을 합쳐서 저장 (Version 2: CollisionShape 포함)
    // 타일 데이터: x(2) + y(2) + type(1) + shape(1) = 6 bytes per tile
    struct TileDataV2 {
        uint16_t x, y;
        uint8_t type;
        uint8_t shape;
    };
    std::vector<TileDataV2> nonEmptyTiles;

    for (const auto& layer : m_layers) {
        if (!layer.visible) continue;
        for (int y = 0; y < m_mapHeight && y < static_cast<int>(layer.tiles.size()); ++y) {
            for (int x = 0; x < m_mapWidth && x < static_cast<int>(layer.tiles[y].size()); ++x) {
                const EditorTile& tile = layer.tiles[y][x];
                if (tile.type != TileType::Empty) {
                    nonEmptyTiles.push_back({
                        static_cast<uint16_t>(x),
                        static_cast<uint16_t>(y),
                        static_cast<uint8_t>(tile.type),
                        static_cast<uint8_t>(tile.shape)
                    });
                }
            }
        }
    }

    uint32_t tileCount = static_cast<uint32_t>(nonEmptyTiles.size());
    file.write(reinterpret_cast<const char*>(&tileCount), sizeof(tileCount));
    for (const auto& tile : nonEmptyTiles) {
        file.write(reinterpret_cast<const char*>(&tile.x), sizeof(tile.x));
        file.write(reinterpret_cast<const char*>(&tile.y), sizeof(tile.y));
        file.write(reinterpret_cast<const char*>(&tile.type), sizeof(tile.type));
        file.write(reinterpret_cast<const char*>(&tile.shape), sizeof(tile.shape));
    }

    // Player Spawn (타일 좌표 그대로 저장)
    int32_t spawnX = m_playerSpawn.x;
    int32_t spawnY = m_playerSpawn.y;
    file.write(reinterpret_cast<const char*>(&spawnX), sizeof(spawnX));
    file.write(reinterpret_cast<const char*>(&spawnY), sizeof(spawnY));

    // Enemy Spawns (타일 좌표 그대로 저장)
    uint32_t enemyCount = static_cast<uint32_t>(m_enemySpawns.size());
    file.write(reinterpret_cast<const char*>(&enemyCount), sizeof(enemyCount));
    for (const auto& spawn : m_enemySpawns) {
        int32_t ex = spawn.x;
        int32_t ey = spawn.y;
        uint8_t et = 0;  // 기본 적 타입
        file.write(reinterpret_cast<const char*>(&ex), sizeof(ex));
        file.write(reinterpret_cast<const char*>(&ey), sizeof(ey));
        file.write(reinterpret_cast<const char*>(&et), sizeof(et));
    }

    m_currentFilename = filename;
    m_hasUnsavedChanges = false;
    std::cout << "Saved: " << filename << std::endl;
}

void Editor::loadMap(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to load: " << filename << std::endl;
        return;
    }

    // Header
    char magic[4];
    file.read(magic, 4);
    if (magic[0] != FILE_MAGIC[0] || magic[1] != FILE_MAGIC[1] ||
        magic[2] != FILE_MAGIC[2] || magic[3] != FILE_MAGIC[3]) {
        std::cerr << "Invalid file format" << std::endl;
        return;
    }

    uint16_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != FILE_VERSION && version != FILE_VERSION_1) {
        std::cerr << "Unsupported version: " << version << std::endl;
        return;
    }

    uint16_t gridSize;
    file.read(reinterpret_cast<char*>(&gridSize), sizeof(gridSize));
    m_gridSize = gridSize;

    uint32_t width, height;
    file.read(reinterpret_cast<char*>(&width), sizeof(width));
    file.read(reinterpret_cast<char*>(&height), sizeof(height));

    // 맵 초기화
    m_mapWidth = static_cast<int>(width);
    m_mapHeight = static_cast<int>(height);
    m_layers.clear();
    addLayer("Ground");

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
        uint8_t ts = static_cast<uint8_t>(CollisionShape::Full);  // 기본값
        if (version == FILE_VERSION) {
            file.read(reinterpret_cast<char*>(&ts), sizeof(ts));
        } else {
            // Version 1: 타입에 따라 기본 CollisionShape 설정
            if (static_cast<TileType>(tt) == TileType::Platform) {
                ts = static_cast<uint8_t>(CollisionShape::Platform);
            }
        }

        if (ty < m_layers[0].tiles.size() && tx < m_layers[0].tiles[ty].size()) {
            m_layers[0].tiles[ty][tx].type = static_cast<TileType>(tt);
            m_layers[0].tiles[ty][tx].shape = static_cast<CollisionShape>(ts);
        }
    }

    // Player Spawn (타일 좌표 그대로 사용)
    int32_t spawnX, spawnY;
    file.read(reinterpret_cast<char*>(&spawnX), sizeof(spawnX));
    file.read(reinterpret_cast<char*>(&spawnY), sizeof(spawnY));
    m_playerSpawn = {spawnX, spawnY};

    // Enemy Spawns (타일 좌표 그대로 사용)
    m_enemySpawns.clear();
    uint32_t enemyCount;
    file.read(reinterpret_cast<char*>(&enemyCount), sizeof(enemyCount));
    for (uint32_t i = 0; i < enemyCount; ++i) {
        int32_t ex, ey;
        uint8_t et;
        file.read(reinterpret_cast<char*>(&ex), sizeof(ex));
        file.read(reinterpret_cast<char*>(&ey), sizeof(ey));
        file.read(reinterpret_cast<char*>(&et), sizeof(et));
        m_enemySpawns.push_back({ex, ey});
    }

    m_currentFilename = filename;
    m_hasUnsavedChanges = false;
    // 맵의 중앙으로 카메라 설정
    m_cameraPos = {
        static_cast<float>(m_mapWidth * m_gridSize) / 2.f,
        static_cast<float>(m_mapHeight * m_gridSize) / 2.f
    };
    m_mapView.setCenter(m_cameraPos);
    std::cout << "Loaded: " << filename << std::endl;
}
