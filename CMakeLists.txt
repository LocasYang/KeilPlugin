cmake_minimum_required(VERSION 3.21)

project(KeilPluginStudio VERSION 1.4.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.5)

if (WIN32)
    enable_language(RC)
endif()

qt_add_executable(KeilPluginStudio
    src/main.cpp
    src/windowchrome.cpp
    src/windowchrome.h
    src/mapparser.cpp
    src/mapparser.h
    src/gnumapparser.cpp
    src/gnumapparser.h
    src/novoproject.cpp
    src/novoproject.h
    src/keilproject.cpp
    src/keilproject.h
    src/uvprojxhook.cpp
    src/uvprojxhook.h
    src/hexmerger.cpp
    src/hexmerger.h
    src/keilplugin.cpp
    src/keilplugin.h
    src/builtinplugins.cpp
    src/builtinplugins.h
    src/pluginregistry.cpp
    src/pluginregistry.h
    src/pluginmanager.cpp
    src/pluginmanager.h
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/I18n.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(KeilPluginStudio
    URI KeilPluginStudio
    VERSION 1.0
    RESOURCES
        qml/assets/keilplugin-mark.svg
    QML_FILES
        qml/Main.qml
        qml/I18n.qml
        qml/Theme.qml
        qml/components/AppButton.qml
        qml/components/AppCard.qml
        qml/components/AppComboBox.qml
        qml/components/NavItem.qml
        qml/components/StatusPill.qml
        qml/components/DataUnavailableState.qml
        qml/dialogs/ConfirmDialog.qml
        qml/pages/PluginsPage.qml
        qml/pages/ReportPage.qml
        qml/pages/LogPage.qml
        qml/pages/SettingsPage.qml
)

target_link_libraries(KeilPluginStudio PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
)

# GUI 子系统：启动不弹控制台窗口。CLI 模式（--size-report 等）由 Keil/调用方
# 重定向 stdout 时依然正常输出。
set_target_properties(KeilPluginStudio PROPERTIES WIN32_EXECUTABLE TRUE)

if (WIN32)
    target_sources(KeilPluginStudio PRIVATE resources/appicon.rc)
    target_link_libraries(KeilPluginStudio PRIVATE dwmapi)
endif()

if (BUILD_TESTING)
    add_executable(KeilPluginCoreTest
        tests/keilplugintests.cpp
        tests/novotests.cpp
        src/mapparser.cpp
        src/gnumapparser.cpp
        src/novoproject.cpp
        src/keilproject.cpp
        src/uvprojxhook.cpp
        src/hexmerger.cpp
        src/keilplugin.cpp
        src/builtinplugins.cpp
        src/pluginregistry.cpp
    )
    target_include_directories(KeilPluginCoreTest PRIVATE src)
    set_target_properties(KeilPluginCoreTest PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_libraries(KeilPluginCoreTest PRIVATE Qt6::Core)
    add_test(NAME KeilPluginCoreTest COMMAND KeilPluginCoreTest)
    set_tests_properties(KeilPluginCoreTest PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
endif()

set(KEIL_PLUGIN_STUDIO_DIST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dist/KeilPluginStudio")
add_custom_command(TARGET KeilPluginStudio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${KEIL_PLUGIN_STUDIO_DIST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:KeilPluginStudio>"
            "${KEIL_PLUGIN_STUDIO_DIST_DIR}/$<TARGET_FILE_NAME:KeilPluginStudio>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
            "${KEIL_PLUGIN_STUDIO_DIST_DIR}/README.md"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/README_zh-CN.md"
            "${KEIL_PLUGIN_STUDIO_DIST_DIR}/README_zh-CN.md"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/shots"
            "${KEIL_PLUGIN_STUDIO_DIST_DIR}/shots"
    COMMENT "Synchronizing KeilPluginStudio.exe to dist"
    VERBATIM
)

install(TARGETS KeilPluginStudio
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
