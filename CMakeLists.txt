cmake_minimum_required(VERSION 3.21)
project(LOQRGBStudio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
include(GNUInstallDirs)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets)

add_library(loqcore
    src/profile.cpp src/profile.h
    src/animationengine.cpp src/animationengine.h
    src/animationpresets.cpp src/animationpresets.h
    src/colorgradient.cpp src/colorgradient.h
    src/themestore.cpp src/themestore.h
    src/keyboardlayout.cpp src/keyboardlayout.h
    src/keyboardzonemap.cpp src/keyboardzonemap.h
    src/keyboardinputlistener.cpp src/keyboardinputlistener.h
    src/hardware/keyboardbackend.cpp src/hardware/keyboardbackend.h
    src/hardware/ite8295protocol.cpp src/hardware/ite8295protocol.h
    src/hardware/linuxhidrawtransport.cpp)
target_include_directories(loqcore PUBLIC src)
target_link_libraries(loqcore PUBLIC Qt6::Core Qt6::Gui)

add_executable(loq-rgb-studio src/main.cpp src/mainwindow.cpp src/mainwindow.h src/keyboardpreview.cpp src/keyboardpreview.h)
target_link_libraries(loq-rgb-studio PRIVATE loqcore Qt6::Widgets)
target_compile_definitions(loq-rgb-studio PRIVATE LOQ_APP_VERSION="${PROJECT_VERSION}")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(ite8295-hid-read tools/ite8295_hid_read.cpp)
endif()

install(TARGETS loq-rgb-studio RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/loq-rgb-studio.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/loq-rgb-studio.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES packaging/loq-rgb-studio.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Test)
    add_executable(loq-tests tests/tst_core.cpp)
    target_link_libraries(loq-tests PRIVATE loqcore Qt6::Test)
    add_test(NAME core COMMAND loq-tests)
endif()

# -----------------------------------------------------------------------------
# Packaging (CPack)
# -----------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "loq-rgb-studio")
set(CPACK_PACKAGE_VENDOR "LOQ RGB Studio")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Four-zone RGB keyboard lighting studio for Lenovo LOQ laptops")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT "Parthiv Das <parthiv-das@users.noreply.github.com>")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_GENERATOR "DEB;TGZ")

# Debian (.deb) configuration
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "LOQ RGB Studio Developers <loq-rgb-studio@local>")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

include(CPack)
