cmake_minimum_required(VERSION 3.10)
project(Cyber_Landa VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 自动化工具
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 查找 Qt 组件（支持 Qt6）
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets 
    Charts 
    Sql
)

# 手动配置 SQLite3（如果系统没有安装）
set(SQLITE3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3" CACHE PATH "SQLite3 include directory")

# 添加 SQLite3 源文件（作为 C 语言编译）
add_library(sqlite3_lib STATIC ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/sqlite3.c)
set_target_properties(sqlite3_lib PROPERTIES LINKER_LANGUAGE C)
target_include_directories(sqlite3_lib PUBLIC ${SQLITE3_INCLUDE_DIR})

# 收集所有源文件
file(GLOB CORE_SOURCES "src/core/*.cpp")
file(GLOB UI_SOURCES "src/ui/*.cpp")
file(GLOB CORE_HEADERS "src/core/*.h")
file(GLOB UI_HEADERS "src/ui/*.h")
file(GLOB UI_FILES "src/ui/*.ui")

# 创建可执行文件
add_executable(${PROJECT_NAME} 
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${CORE_HEADERS}
    ${UI_HEADERS}
    ${UI_FILES}
)

# 链接 Qt 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
    Qt6::Sql
    sqlite3_lib
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${SQLITE3_INCLUDE_DIR}
)

# Windows 特定配置（隐藏控制台窗口）
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()

# 部署 Qt DLL（Windows）
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    get_target_property(QT6_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT6_WINDEPLOYQT_EXECUTABLE ${QT6_QMAKE_EXECUTABLE} PATH)
    set(QT6_WINDEPLOYQT_EXECUTABLE "${QT6_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${QT6_WINDEPLOYQT_EXECUTABLE} --no-translations $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Running windeployqt..."
    )
endif()