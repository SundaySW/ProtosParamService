cmake_minimum_required(VERSION 3.16)

#set(CMAKE_TOOLCHAIN_FILE cmake/toolchain.cmake)

project(ProtosParamService C CXX ASM)
add_definitions(-D_VER="V1.0")
add_definitions(-D_BUILD_TYPE_="${CMAKE_BUILD_TYPE}")

message("SysName: ${CMAKE_SYSTEM_NAME}")
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
endif()
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_compile_options(
            -finput-charset=CP1251
            -fexec-charset=CP866
    )
#    enable_language("RC")
#    set (WIN32_RESOURCES Resource/resource.rc)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(qt_ver      Qt5)
find_package(${qt_ver}Widgets     REQUIRED)
find_package(${qt_ver}Gui         REQUIRED)
find_package(${qt_ver}Core        REQUIRED)
find_package(${qt_ver}Network     REQUIRED)
find_package(${qt_ver}SerialPort  REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Monitor/*.*
        ${CMAKE_CURRENT_SOURCE_DIR}/DB_Driver/*.*
        ${CMAKE_CURRENT_SOURCE_DIR}/view_TableModel/*.*
        ${CMAKE_CURRENT_SOURCE_DIR}/Param_Item/*.*
        ${CMAKE_CURRENT_SOURCE_DIR}/view_Settings/*.*
        ${CMAKE_CURRENT_SOURCE_DIR}/view_ParamTabs/*.*
        )

set(SOURCE_FILES
        ParamService.cpp
        ${WIN32_RESOURCES}
        ${SOURCES}
        )

add_executable(${PROJECT_NAME}
        ${SOURCE_FILES}
        ${WIN32_RESOURCES}
        )

IF(CMAKE_BUILD_TYPE MATCHES Release)
    message("Release build!")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.json ${CMAKE_CURRENT_BINARY_DIR}/saved_params.json COPYONLY)
    add_custom_command(
            TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/qss/ ${CMAKE_CURRENT_BINARY_DIR}/qss/
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/logs/ ${CMAKE_CURRENT_BINARY_DIR}/logs/
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/appLogs/ ${CMAKE_CURRENT_BINARY_DIR}/appLogs/
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/psql_libs/lib ${CMAKE_CURRENT_BINARY_DIR}/
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Resource/ ${CMAKE_CURRENT_BINARY_DIR}/Resource/
    )
    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE true)
        add_custom_command(
                TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND windeployqt "$<TARGET_FILE:${PROJECT_NAME}>"
                VERBATIM
        )
    endif()
ENDIF()

#target_compile_definitions(${PROJECT_NAME}
#        PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Monitor
        ${CMAKE_CURRENT_SOURCE_DIR}/DB_Driver
        ${CMAKE_CURRENT_SOURCE_DIR}/view_TableModel
        ${CMAKE_CURRENT_SOURCE_DIR}/Param_Item
        ${CMAKE_CURRENT_SOURCE_DIR}/view_Settings/
        ${CMAKE_CURRENT_SOURCE_DIR}/view_ParamTabs/
        )

target_sources(${PROJECT_NAME}
        PRIVATE
        ${SOURCE_FILES}
        )

target_link_libraries(${PROJECT_NAME} ${qt_ver}::Core)
target_link_libraries(${PROJECT_NAME} ${qt_ver}::Gui)
target_link_libraries(${PROJECT_NAME} ${qt_ver}::Widgets)
target_link_libraries(${PROJECT_NAME} ${qt_ver}::Network)
target_link_libraries(${PROJECT_NAME} ${qt_ver}::SerialPort)