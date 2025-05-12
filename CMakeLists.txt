cmake_minimum_required(VERSION 3.20)
project(pokebattle C)

set(CMAKE_C_STANDARD 11)

# Configurar diretórios das bibliotecas
set(RAYLIB_DIR "G:/raylib")  # Ajuste este caminho
set(CURL_DIR "G:/curl")      # Ajuste este caminho

# Incluir diretórios de headers
include_directories(
        include
        include/render
        ${RAYLIB_DIR}/include
        ${CURL_DIR}/include
)

# Adicionar diretórios de bibliotecas
link_directories(
        ${RAYLIB_DIR}/lib
        ${CURL_DIR}/lib
)

# Coletar todos os arquivos fonte
file(GLOB_RECURSE SOURCES
        "src/*.c"
        "src/render/*.c"
)

# Criar o executável
add_executable(pokebattle ${SOURCES}
        src/scaling.c
        include/scaling.h
        src/decision_tree.c
        include/decision_tree.h)

# Linkar bibliotecas necessárias no Windows
target_link_libraries(pokebattle
        raylib
        opengl32
        gdi32
        winmm
        curl
        ws2_32
        wldap32
        crypt32
        normaliz
)

# Copiar recursos para o diretório de build
add_custom_command(TARGET pokebattle POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        $<TARGET_FILE_DIR:pokebattle>/resources
)

# Copiar DLLs necessárias apenas no Windows
if(WIN32)
    # Copiar todas as DLLs do diretório bin do Raylib
    file(GLOB RAYLIB_DLLS "${RAYLIB_DIR}/bin/*.dll")
    foreach(DLL ${RAYLIB_DLLS})
        add_custom_command(TARGET pokebattle POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL}"
                $<TARGET_FILE_DIR:pokebattle>
        )
    endforeach()

    # Copiar todas as DLLs do diretório bin do cURL
    file(GLOB CURL_DLLS "${CURL_DIR}/bin/*.dll")
    foreach(DLL ${CURL_DLLS})
        add_custom_command(TARGET pokebattle POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL}"
                $<TARGET_FILE_DIR:pokebattle>
        )
    endforeach()

    # Se você souber exatamente quais DLLs precisa, pode especificar manualmente:
    # Lista de DLLs específicas que podem ser necessárias
    set(REQUIRED_DLLS
            "${RAYLIB_DIR}/bin/raylib.dll"
            "${CURL_DIR}/bin/libcurl.dll"
            "${CURL_DIR}/bin/libcurl-4.dll"    # Algumas versões usam este nome
            "${CURL_DIR}/bin/libssl-1_1-x64.dll"
            "${CURL_DIR}/bin/libcrypto-1_1-x64.dll"
            "${CURL_DIR}/bin/libssl-3-x64.dll"
            "${CURL_DIR}/bin/libcrypto-3-x64.dll"
            "${CURL_DIR}/bin/zlib1.dll"
            "${CURL_DIR}/bin/libssh2.dll"
    )

    # Copiar cada DLL se ela existir
    foreach(DLL ${REQUIRED_DLLS})
        if(EXISTS "${DLL}")
            add_custom_command(TARGET pokebattle POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL}"
                    $<TARGET_FILE_DIR:pokebattle>
            )
        endif()
    endforeach()
endif()

# Definir diretório de trabalho para execução
set_property(TARGET pokebattle PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

# Mensagem de debug para verificar os caminhos
message(STATUS "RAYLIB_DIR: ${RAYLIB_DIR}")
message(STATUS "CURL_DIR: ${CURL_DIR}")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")