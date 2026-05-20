cmake_minimum_required(VERSION 3.19)
project(NoisyTyping LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql Test)

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
    databasemanager.h databasemanager.cpp
    featureextractor.h featureextractor.cpp
    profilemodel.h profilemodel.cpp
    verificationstatscsvwriter.h verificationstatscsvwriter.cpp
    verificationresultscsvwriter.h verificationresultscsvwriter.cpp
    thresholdanalysis.h thresholdanalysis.cpp
    thresholdanalysiscsvwriter.h thresholdanalysiscsvwriter.cpp
)

target_link_libraries(NoisyTyping
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Sql
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

qt_add_executable(FeatureExtractorTest
    tests/featureextractortest.cpp
    featureextractor.h
    featureextractor.cpp
    keystrokeevent.h
    typingsession.h
    sessionsummary.h
    sessionfeaturevector.h
)

target_link_libraries(FeatureExtractorTest
    PRIVATE
        Qt::Core
        Qt::Test
)

enable_testing()

add_test(
    NAME FeatureExtractorTest
    COMMAND FeatureExtractorTest
)

qt_add_executable(ProfileModelTest
    tests/profilemodeltest.cpp
    profilemodel.h
    profilemodel.cpp
    sessionfeaturevector.h
)

target_link_libraries(ProfileModelTest
    PRIVATE
        Qt::Core
        Qt::Test
)

add_test(
    NAME ProfileModelTest
    COMMAND ProfileModelTest
)

qt_add_executable(ThresholdAnalysisTest
    tests/thresholdanalysistest.cpp
    thresholdanalysis.h
    thresholdanalysis.cpp
    databasemanager.h
    sessionfeaturevector.h
)

target_link_libraries(ThresholdAnalysisTest
    PRIVATE
        Qt::Core
        Qt::Sql
        Qt::Test
)

add_test(
    NAME ThresholdAnalysisTest
    COMMAND ThresholdAnalysisTest
)

