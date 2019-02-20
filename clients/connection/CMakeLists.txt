project(connectionApp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(connectionApp_SOURCES
    main.cpp
    MainWindow.cpp
)

set(connectionApp_HEADERS
    MainWindow.hpp
)

set(connectionApp_FORMS
    MainWindow.ui
)

set(connectionApp_RESOURCES
    resources.qrc
)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

add_executable(connectionApp-qt${QT_VERSION_MAJOR} ${connectionApp_SOURCES} ${connectionApp_FORMS} ${connectionApp_RESOURCES})

set_target_properties(connectionApp-qt${QT_VERSION_MAJOR} PROPERTIES AUTOMOC TRUE)

target_include_directories(connectionApp-qt${QT_VERSION_MAJOR} PRIVATE
    ${QT5_INCLUDES}
)

target_link_libraries(connectionApp-qt${QT_VERSION_MAJOR} LINK_PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
)

target_link_libraries(connectionApp-qt${QT_VERSION_MAJOR} LINK_PUBLIC
    TelegramQt${QT_VERSION_MAJOR}
)
