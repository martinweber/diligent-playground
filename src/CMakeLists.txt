set(app_header_files_
    hello.h
    cgr_error.h
    StandardOutSink.h
)

set(app_source_files_
    main.cpp
    hello.cpp
)

set(app_shader_files_
    shaders/cube.vsh
    shaders/cube.psh
)

source_group("shaders" FILES ${app_shader_files_})
source_group("src" FILES ${app_source_files_} ${app_header_files_})

add_executable(Hello-Diligent
    ${app_header_files_}
    ${app_source_files_}
    ${app_shader_files_}
)

target_compile_definitions(Hello-Diligent
    PRIVATE
        UNICODE
        ENGINE_DLL=1
        CHANGE_G3LOG_DEBUG_TO_DBUG
)

target_link_libraries(Hello-Diligent
    PRIVATE
        g3log
        glfw
        Diligent-Common
        Diligent-GraphicsTools
        Diligent-GraphicsEngineVk-shared
)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Hello-Diligent)
    set_property(TARGET Hello-Diligent PROPERTY
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Hello-Diligent>"
    # set VS_DEBUGGER_ENVIRONMENT Path to Diligent binaries so that dlls can be loaded
)   

set_source_files_properties(${app_shader_files_} 
    PROPERTIES 
        VS_TOOL_OVERRIDE "None"
        VS_DEPLOYMENT_LOCATION "."
)

add_custom_command(OUTPUT Copy_Shaders_Command
    # COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Hello-Diligent>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:Hello-Diligent>/shaders"
)

add_custom_target(Copy_Shaders DEPENDS Copy_Shaders_Command)
