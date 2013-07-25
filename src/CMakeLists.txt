set(comp_srcs DirectInput.cpp DirectInput8Manager.cpp)
set(standalone_srcs DirectInputComp.cpp)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME})
include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_BINARY_DIR}/idl)
include_directories(${OPENRTM_INCLUDE_DIRS})
include_directories(${OMNIORB_INCLUDE_DIRS})
include_directories(${LIBDXSDK_INC_DIR})

add_definitions(${OPENRTM_CFLAGS})
add_definitions(${OMNIORB_CFLAGS})

link_directories(${OPENRTM_LIBRARY_DIRS})
link_directories(${OMNIORB_LIBRARY_DIRS})
link_directories(${LIBDXSDK_LIB_DIR})

add_library(${PROJECT_NAME} ${LIB_TYPE} ${comp_srcs} ${ALL_IDL_SRCS})
set_source_files_properties(${ALL_IDL_SRCS} PROPERTIES GENERATED 1)
add_dependencies(${PROJECT_NAME} ALL_IDL_TGT)
target_link_libraries(${PROJECT_NAME} ${OPENRTM_LIBRARIES} ${LIBDXSDK_LIBRARIES})

add_executable(${PROJECT_NAME}comp ${standalone_srcs})
target_link_libraries(${PROJECT_NAME}comp ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}comp
    EXPORT ${PROJECT_NAME}
    RUNTIME DESTINATION ${BIN_INSTALL_DIR} COMPONENT component
    LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT component
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT component)
install(EXPORT ${PROJECT_NAME}
    DESTINATION ${LIB_INSTALL_DIR}/${PROJECT_NAME}
    FILE ${PROJECT_NAME}Depends.cmake)

