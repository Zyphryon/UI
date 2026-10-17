## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
##
## This work is licensed under the terms of the MIT license.
##
## For a copy, see <https://opensource.org/licenses/MIT>.
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

CMAKE_MINIMUM_REQUIRED(VERSION 3.26)

## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Project
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

PROJECT(ZyUI VERSION 1.0.0 LANGUAGES CXX)

## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Plugins
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

INCLUDE(FetchContent)
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/CMake/CMakeEmbedTheme.cmake)

## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Dependency (Zyphryon Engine)
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

FetchContent_Declare(External_ZyEngine
        GIT_REPOSITORY https://github.com/Zyphryon/Engine
        GIT_TAG        main
        GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(External_ZyEngine)

## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Modules
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Module/UI.Core)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Module/UI.Data)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Module/UI.Markup)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Module/UI.Gesture)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Module/UI.Control)

## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Target
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

ADD_LIBRARY(${PROJECT_NAME} INTERFACE)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} INTERFACE
        "ZyUI_Core"
        "ZyUI_Data"
        "ZyUI_Markup"
        "ZyUI_Gesture"
        "ZyUI_Control")

## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Example
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

OPTION(ZY_UI_EXAMPLE "Build the example that shows every feature of the interface" ${PROJECT_IS_TOP_LEVEL})

IF (ZY_UI_EXAMPLE)

    ZyAddTarget(${PROJECT_NAME}_Example EXECUTABLE SOURCE "Example/Source")

    TARGET_LINK_LIBRARIES(${PROJECT_NAME}_Example PRIVATE ${PROJECT_NAME})
    TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME}_Example PRIVATE ZY_UI_EXAMPLE_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/Example/Resources")

    ZyApplyOptions(${PROJECT_NAME}_Example)
    ZyApplyResources(${PROJECT_NAME}_Example)
    ZyApplyModules(${PROJECT_NAME}_Example)

ENDIF (ZY_UI_EXAMPLE)

## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Test
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

OPTION(ZY_UI_TEST "Build the tests of the interface" ${PROJECT_IS_TOP_LEVEL})

IF (ZY_UI_TEST)

    ZyAddTarget(${PROJECT_NAME}_Test EXECUTABLE SOURCE "Test/Source")

    TARGET_LINK_LIBRARIES(${PROJECT_NAME}_Test PRIVATE ${PROJECT_NAME})

ENDIF (ZY_UI_TEST)
