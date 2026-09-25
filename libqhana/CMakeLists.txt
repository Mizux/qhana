add_library(libqhana SHARED)
target_sources(libqhana PRIVATE
  qhana.cpp
  qhana.h
)
target_include_directories(libqhana PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qhana>
)
target_compile_features(libqhana PUBLIC cxx_std_20)
target_compile_options(libqhana PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -fstack-protector>
)
set_target_properties(libqhana PROPERTIES
  PREFIX ""
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  POSITION_INDEPENDENT_CODE ON
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  PUBLIC_HEADER qhana.h
)
if(APPLE)
  set_target_properties(libqhana PROPERTIES
    INSTALL_RPATH "@loader_path")
elseif(UNIX AND NOT APPLE)
  set_target_properties(libqhana PROPERTIES
    INSTALL_RPATH "$ORIGIN")
endif()
add_library(${PROJECT_NAMESPACE}::libqhana ALIAS libqhana)

# Install
install(TARGETS libqhana
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qhana
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
