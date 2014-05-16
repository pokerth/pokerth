project(data)

qtcreator_add_project_resources(${CMAKE_CURRENT_SOURCE_DIR})

install(DIRECTORY ./ DESTINATION ${CMAKE_INSTALL_DATADIR}/pokerth/data)

add_custom_target (
    copy_dir ALL
)

add_custom_command(
    TARGET copy_dir
    COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)
