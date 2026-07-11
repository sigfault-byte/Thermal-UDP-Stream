cmake_minimum_required(VERSION 3.16)

project(padawan_viewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Network)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(padawan_viewer
    main.cpp
    models/frame_model.cpp
    models/frame_model.h
    network/udp_receiver.cpp
    network/udp_receiver.h
    protocol/packet_decoder.cpp
    protocol/packet_decoder.h
    protocol/thermal_frame.h
)

qt_add_qml_module(padawan_viewer
    URI PadawanViewer
    QML_FILES
        Main.qml
)

target_link_libraries(padawan_viewer
    PRIVATE Qt6::Quick
    PRIVATE Qt6::Network
)
