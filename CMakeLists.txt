cmake_minimum_required(VERSION 3.19)

project(qfakemail LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

qt_add_executable(qfakemail WIN32 MACOSX_BUNDLE
	main.cpp
	qfakemail.cpp qfakemail.h
	qfakemailwidget.ui
	qfakemail.qrc
)

if(WIN32)
	target_sources(qfakemail PRIVATE qfakemail.rc)
endif()

target_link_libraries(qfakemail PRIVATE Qt6::Widgets Qt6::Network)
target_compile_definitions(qfakemail PRIVATE QT_USE_QSTRINGBUILDER)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(qfakemail PRIVATE -Wall -Wextra)
endif()

install(TARGETS qfakemail
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
