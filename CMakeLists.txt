cmake_minimum_required(VERSION 3.19)
project(NoisyTyping LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(NoisyTyping
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    typingtextedit.h
    typingtextedit.cpp
    keystrokeevent.h
    typingsession.h
    sessionsummary.h
    sessionfeaturevector.h
    sessionfeaturecsvwriter.h
    sessionfeaturecsvwriter.cpp
    sessioneventcsvwriter.h
    sessioneventcsvwriter.cpp
)

target_link_libraries(NoisyTyping
    PRIVATE
        Qt::Core
        Qt::Widgets
)

target_compile_definitions(NoisyTyping PRIVATE
    NOISYTYPING_PROJECT_DIR="${CMAKE_SOURCE_DIR}"
)

include(GNUInstallDirs)

install(TARGETS NoisyTyping
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET NoisyTyping
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
