cmake_minimum_required(VERSION 3.21)

project(Mach3SurfacingGenerator VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(CTest)
include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(MACH3_APP_SOURCES
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/GCodeGenerator.h
    src/GCodeGenerator.cpp
    src/Mach3PostProcessor.h
    src/Mach3PostProcessor.cpp
    src/PresetManager.h
    src/PresetManager.cpp
    src/PreviewWidget.h
    src/PreviewWidget.cpp
    src/SurfacingJob.h
    src/ToolPreset.h
    src/MaterialPreset.h
    resources/resources.qrc
)

if(WIN32)
    configure_file(
        assets/windows.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/windows.rc
        @ONLY
    )
    list(APPEND MACH3_APP_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/windows.rc)
endif()

qt_add_executable(Mach3SurfacingGenerator WIN32
    ${MACH3_APP_SOURCES}
)

target_link_libraries(Mach3SurfacingGenerator PRIVATE Qt6::Widgets)
target_include_directories(Mach3SurfacingGenerator PRIVATE src)
target_compile_definitions(Mach3SurfacingGenerator PRIVATE
    MACH3_APP_AUTHOR="Nicolas Truffier"
    MACH3_APP_VERSION="${PROJECT_VERSION}"
)

if(MSVC)
    target_compile_options(Mach3SurfacingGenerator PRIVATE /W4 /permissive-)
else()
    target_compile_options(Mach3SurfacingGenerator PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_command(TARGET Mach3SurfacingGenerator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:Mach3SurfacingGenerator>/presets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/presets/presets.json"
            "$<TARGET_FILE_DIR:Mach3SurfacingGenerator>/presets/presets.json"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:Mach3SurfacingGenerator>/output"
)

install(TARGETS Mach3SurfacingGenerator
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
    BUNDLE DESTINATION .
)
install(FILES LICENSE README.md CHANGELOG.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/Mach3SurfacingGenerator
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(FILES presets/presets.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Mach3SurfacingGenerator
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

if(UNIX AND NOT APPLE)
    configure_file(
        packaging/linux/Mach3SurfacingGenerator.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/Mach3SurfacingGenerator.desktop
        @ONLY
    )
    configure_file(
        packaging/linux/io.github.nicolastruffier.Mach3SurfacingGenerator.metainfo.xml.in
        ${CMAKE_CURRENT_BINARY_DIR}/io.github.nicolastruffier.Mach3SurfacingGenerator.metainfo.xml
        @ONLY
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Mach3SurfacingGenerator.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
    install(FILES assets/logo-256.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        RENAME Mach3SurfacingGenerator.png
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
    install(FILES assets/logo-512.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps
        RENAME Mach3SurfacingGenerator.png
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/io.github.nicolastruffier.Mach3SurfacingGenerator.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
endif()

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)

    qt_add_executable(GCodeGeneratorTests
        tests/GCodeGeneratorTests.cpp
        src/GCodeGenerator.h
        src/GCodeGenerator.cpp
        src/Mach3PostProcessor.h
        src/Mach3PostProcessor.cpp
        src/SurfacingJob.h
        src/ToolPreset.h
        src/MaterialPreset.h
    )

    target_link_libraries(GCodeGeneratorTests PRIVATE Qt6::Test Qt6::Core)
    target_include_directories(GCodeGeneratorTests PRIVATE src)

    add_test(NAME GCodeGeneratorTests COMMAND GCodeGeneratorTests)
endif()

set(CPACK_PACKAGE_NAME "Mach3SurfacingGenerator")
set(CPACK_PACKAGE_VENDOR "Nicolas Truffier")
set(CPACK_PACKAGE_CONTACT "nicolas.truffier@atechsys.fr")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Générateur open source de G-code de surfaçage pour Mach3"
)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME
        "Mach3SurfacingGenerator-${PROJECT_VERSION}-windows-x64"
    )
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;TGZ")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Nicolas Truffier")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
    )
    set(CPACK_PACKAGE_FILE_NAME
        "Mach3SurfacingGenerator-${PROJECT_VERSION}-linux-x86_64"
    )
endif()

include(CPack)
