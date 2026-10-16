cmake_minimum_required(VERSION 3.21)

project(ovpn3-gui VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Network LinguistTools)
qt_standard_project_setup()

qt_add_executable(ovpn3-gui WIN32
    src/main.cpp
    src/AuthDialog.cpp
    src/AuthDialog.h
    src/Connection.cpp
    src/Connection.h
    src/ConnectionManager.cpp
    src/ConnectionManager.h
    src/ConnectionsWindow.cpp
    src/ConnectionsWindow.h
    src/CredentialStore.cpp
    src/CredentialStore.h
    src/Icons.cpp
    src/Icons.h
    src/ManagementClient.cpp
    src/ManagementClient.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/StatusWindow.cpp
    src/StatusWindow.h
    src/TrafficGraph.cpp
    src/TrafficGraph.h
    src/TrayController.cpp
    src/TrayController.h
)

target_compile_definitions(ovpn3-gui PRIVATE OVPN3_GUI_VERSION="${PROJECT_VERSION}")
target_link_libraries(ovpn3-gui PRIVATE Qt6::Widgets Qt6::Network)
if (WIN32)
    target_link_libraries(ovpn3-gui PRIVATE crypt32)
endif()

if (MSVC)
    target_compile_options(ovpn3-gui PRIVATE /W4 /utf-8)
endif()

# Translations, embedded under :/i18n and picked by the system locale.
# Run the update_translations target after changing user visible strings.
set(OVPN3_GUI_LANGUAGES ru)
set(OVPN3_GUI_TS_FILES)
foreach (lang IN LISTS OVPN3_GUI_LANGUAGES)
    list(APPEND OVPN3_GUI_TS_FILES translations/ovpn3-gui_${lang}.ts)
endforeach()
qt_add_translations(ovpn3-gui
    TS_FILES ${OVPN3_GUI_TS_FILES}
    RESOURCE_PREFIX /i18n
    LUPDATE_OPTIONS -locations none -no-obsolete)

# Qt's own strings (standard buttons, file dialogs) for the same languages
set(_qt_tr_dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS}")
set(_qt_qm_files)
foreach (lang IN LISTS OVPN3_GUI_LANGUAGES)
    if (EXISTS "${_qt_tr_dir}/qtbase_${lang}.qm")
        list(APPEND _qt_qm_files "${_qt_tr_dir}/qtbase_${lang}.qm")
    endif()
endforeach()
if (_qt_qm_files)
    qt_add_resources(ovpn3-gui qt_translations PREFIX /i18n BASE "${_qt_tr_dir}" FILES ${_qt_qm_files})
endif()

# The workers (omicliagent.exe, omicli.exe) come from the core build.
# Copy them next to the GUI, where it looks for omicliagent.exe by default.
set(OVPN3_WORKER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/msvc/amd64/openvpn/omi/Release"
    CACHE PATH "Directory with omicliagent.exe and its DLLs")

if (EXISTS "${OVPN3_WORKER_DIR}/omicliagent.exe")
    file(GLOB OVPN3_WORKER_FILES "${OVPN3_WORKER_DIR}/*.exe" "${OVPN3_WORKER_DIR}/*.dll")
    add_custom_command(TARGET ovpn3-gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OVPN3_WORKER_FILES} $<TARGET_FILE_DIR:ovpn3-gui>
        COMMENT "Copying OpenVPN3 workers")
else()
    message(WARNING "omicliagent.exe not found in OVPN3_WORKER_DIR=${OVPN3_WORKER_DIR}; "
                    "set the worker path in the GUI settings")
endif()

# Make the build directory runnable without installing Qt system-wide
if (WIN32)
    get_target_property(_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin "${_qmake}" DIRECTORY)
    find_program(WINDEPLOYQT windeployqt HINTS "${_qt_bin}")
    if (WINDEPLOYQT)
        add_custom_command(TARGET ovpn3-gui POST_BUILD
            COMMAND "${WINDEPLOYQT}" --no-translations --no-system-d3d-compiler --no-opengl-sw
                    $<IF:$<CONFIG:Debug>,--debug,--release> "$<TARGET_FILE:ovpn3-gui>"
            COMMENT "Deploying Qt runtime")
    endif()
endif()
