cmake_minimum_required(VERSION 3.13)

project(ManifoldEditor)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(wxWidgets REQUIRED base core aui gl propgrid stc xml)
find_package(Freetype REQUIRED MODULE)
find_package(PkgConfig REQUIRED)
pkg_search_module(GTK3 REQUIRED gtk+-3.0)

# Doxygen 1.13.2
find_package(Doxygen REQUIRED)

# Add a custom target for generating documentation
add_custom_target(doc ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating documentation with Doxygen"
    VERBATIM
)

# Ensure documentation is built by default
add_dependencies(doc ManifoldEditor)

add_executable(ManifoldEditor
    ../../src/editor/AudioSystem.cpp
    ../../src/editor/BrowserWindow.cpp
    ../../src/editor/CGridSceneNode.cpp
    ../../src/editor/Commands.cpp
    ../../src/editor/CSceneNodeAnimatorCameraOrtho.cpp
    ../../src/editor/Editor.cpp
    ../../src/editor/Entry.cpp
    ../../src/editor/ExplorerPanel.cpp
    ../../src/editor/FSHandler.cpp
    ../../src/editor/MainWindow.cpp
    ../../src/editor/Map.cpp
    ../../src/editor/MapEditor.cpp
    ../../src/editor/MpkFSHandler.cpp
    ../../src/editor/PackageManager.cpp
    ../../src/editor/PlayProcess.cpp
    ../../src/editor/Preferences.cpp
    ../../src/editor/ProjectEditor.cpp
    ../../src/editor/ProjectExplorer.cpp
    ../../src/editor/PropertyPanel.cpp
    ../../src/editor/ScriptEditor.cpp
    ../../src/editor/Serialize.cpp
    ../../src/editor/ViewPanel.cpp
    ../../src/extend/CylinderSceneNode.cpp
    ../../src/extend/PathSceneNode.cpp
    ../../src/extend/PlaneSceneNode.cpp
    ../../src/extend/PlayerStartNode.cpp
    ../../src/extend/SceneNodeFactory.cpp
    ../../thirdparty/ktkr3d/CGUITTFont.cpp)

include(${wxWidgets_USE_FILE})

target_include_directories(ManifoldEditor PRIVATE
    ${FREETYPE_INCLUDE_DIRS}
    ${GTK3_INCLUDE_DIRS}
    ../../src/editor
    ../../src/extend
    ../../thirdparty/ktkr3d
    ../../thirdparty/irrlicht-1.8.5/include
    ../../thirdparty/miniaudio-0.11.22)

target_compile_options(ManifoldEditor PRIVATE ${GTK3_CFLAGS_OTHER} -fPIE)
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,-rpath='$ORIGIN'")

target_link_directories(ManifoldEditor PUBLIC ../../bin)
target_link_libraries(ManifoldEditor ${wxWidgets_LIBRARIES} ${FREETYPE_LIBRARIES} ${GTK3_LIBRARIES} 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bin/libIrrlicht.so.1.8)
