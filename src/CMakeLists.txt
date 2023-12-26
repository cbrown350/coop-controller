# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources}
                    # EMBED_FILES ${project_dir}/.pio/build/certs/ca_certs.bin
                    # EMBED_TXTFILES ${project_dir}/.pio/build/certs/ca_cert.pem
                    EMBED_TXTFILES ${project_dir}/.pio/build/timezone_data/zones.csv
                    INCLUDE_DIRS ".")
