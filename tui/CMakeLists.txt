set(CURSES_NEED_NCURSES TRUE)
set(CURSES_NEED_WIDE TRUE)
find_package(Curses)

if(NOT CURSES_FOUND)
  message(STATUS "Curses not found, skipping qhana-tui build")
  return()
endif()

add_executable(qhana-tui)
target_sources(qhana-tui PRIVATE
  main.cpp
  tui.cpp
  tui.h
)
target_include_directories(qhana-tui PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/app>
  ${CURSES_INCLUDE_DIRS}
)
target_compile_definitions(qhana-tui PRIVATE
  _DEFAULT_SOURCE
  _XOPEN_SOURCE=600
)
target_compile_features(qhana-tui PRIVATE cxx_std_20)
target_compile_options(qhana-tui PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -fstack-protector>
)
set_target_properties(qhana-tui PROPERTIES
  VERSION ${PROJECT_VERSION}
  POSITION_INDEPENDENT_CODE ON
)
if(APPLE)
  set_target_properties(qhana-tui PROPERTIES
    INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
elseif(UNIX AND NOT APPLE)
  set_target_properties(qhana-tui PROPERTIES
    INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()

target_link_libraries(qhana-tui PRIVATE
  ${PROJECT_NAMESPACE}::libqhana
  ${CURSES_LIBRARIES}
)
add_executable(${PROJECT_NAMESPACE}::qhana-tui ALIAS qhana-tui)

# Install
install(TARGETS qhana-tui
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
