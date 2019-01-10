cmake_minimum_required(VERSION 3.5)

project(bayan VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})

add_executable(bayan main.cpp)

find_package(Boost COMPONENTS program_options filesystem system REQUIRED)

include_directories(${Boost_INCLUDE_DIR})

set_target_properties(bayan PROPERTIES
	CXX_STANDARD 14
	CXX_STANDARD_REQUIRED ON
	COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra"
)

target_link_libraries(bayan
	${Boost_LIBRARIES}
)

install(TARGETS bayan RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT sava_test@mail.ru)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include (CPack)
