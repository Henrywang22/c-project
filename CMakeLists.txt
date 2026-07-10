cmake_minimum_required(VERSION 3.21)

project(FishingVoyage VERSION 1.0.4 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

# 正式包也保留开发者测试入口；测试成绩不会写入排行榜。
option(FISHINGVOYAGE_ENABLE_TEST_MODE "Enable the in-game developer test mode" ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

set(FISHING_VOYAGE_SOURCES
    main.cpp
    BackpackDialog.cpp
    boss.cpp
    EncyclopediaDialog.cpp
    Enemy.cpp
    Filemanager.cpp
    Fish.cpp
    GameManager.cpp
    GameUiDialog.cpp
    GameWindow.cpp
    InventorySystem.cpp
    Item.cpp
    obstacle.cpp
    player.cpp
    Shopdialog.cpp
    wavesystem.cpp
    Weapon.cpp
    weathersystem.cpp
)

set(FISHING_VOYAGE_HEADERS
    BackpackDialog.h
    Boss.h
    EncyclopediaDialog.h
    Enemy.h
    FileManager.h
    Fish.h
    GameConfig.h
    GameManager.h
    GameUiDialog.h
    GameWindow.h
    InventorySystem.h
    Item.h
    ItemFactory.h
    Obstacle.h
    Player.h
    Shopdialog.h
    SimulationClock.h
    WaveSystem.h
    Weapon.h
    WeatherSystem.h
)

qt_add_executable(FishingVoyage WIN32
    ${FISHING_VOYAGE_SOURCES}
    ${FISHING_VOYAGE_HEADERS}
)

if(WIN32)
    target_sources(FishingVoyage PRIVATE app_icon.rc)
endif()

target_link_libraries(FishingVoyage PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
target_compile_definitions(FishingVoyage PRIVATE
    QT_NO_DEBUG
    QT_NO_DEPRECATED_WARNINGS
    FISHINGVOYAGE_VERSION="${PROJECT_VERSION}"
)

if(FISHINGVOYAGE_ENABLE_TEST_MODE)
    target_compile_definitions(FishingVoyage PRIVATE FISHINGVOYAGE_ENABLE_TEST_MODE)
endif()

if(MSVC)
    target_compile_options(FishingVoyage PRIVATE /W4 /utf-8 /permissive-)
else()
    target_compile_options(FishingVoyage PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Build the large Qt resource collection as an external binary resource.
# This keeps the executable small and avoids compiling a ~500 MB generated
# qrc_FishingVoyage.cpp translation unit.
get_target_property(QT_RCC_EXECUTABLE Qt6::rcc IMPORTED_LOCATION)
set(FISHING_VOYAGE_RCC "${CMAKE_CURRENT_BINARY_DIR}/FishingVoyage.rcc")
add_custom_command(
    OUTPUT "${FISHING_VOYAGE_RCC}"
    COMMAND "${QT_RCC_EXECUTABLE}" --binary "${CMAKE_CURRENT_SOURCE_DIR}/FishingVoyage.qrc"
            -o "${FISHING_VOYAGE_RCC}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/FishingVoyage.qrc"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building FishingVoyage.rcc"
    VERBATIM
)
add_custom_target(FishingVoyageResources ALL DEPENDS "${FISHING_VOYAGE_RCC}")
add_dependencies(FishingVoyage FishingVoyageResources)

install(TARGETS FishingVoyage RUNTIME DESTINATION .)
install(FILES "${FISHING_VOYAGE_RCC}" DESTINATION .)
