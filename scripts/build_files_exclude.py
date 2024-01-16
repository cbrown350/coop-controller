# Concepts in https://github.com/platformio/platformio-core/blob/develop/platformio/project/config.py
# could be in
# https://docs.platformio.org/en/latest/projectconf/sections/platformio/options/directory/shared_dir.html#projectconf-pio-shared-dir
# shared_dir

Import("env")

must_exist=False

# Get custom_ list of files to skip
#https://docs.platformio.org/en/latest/scripting/middlewares.html

custom_build_files_exclude = env.GetProjectOption("custom_build_files_exclude")
if custom_build_files_exclude is not None:    
    print(" ** Custom_ skip build targets** ", custom_build_files_exclude )

    def skip_tgt_from_build(env, node):
        # to ignore file from a build process, just return None
        return None

    # iterate over all files
    temp = custom_build_files_exclude.split(" ")
    for value in temp:
        env.AddBuildMiddleware(skip_tgt_from_build, value)