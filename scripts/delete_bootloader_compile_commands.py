import os
Import("env")

build_dir = env.subst("$BUILD_DIR")
bootloader_compile_command_path = os.path.join(build_dir, "bootloader", "compile_commands.json")
def remove_unneeded_compile_commands(source, target, env):
    print("Removing bootloader compile command path: {}".format(bootloader_compile_command_path))
    if os.path.exists(bootloader_compile_command_path):
        os.remove(bootloader_compile_command_path)
    
env.AddPostAction("checkprogsize", remove_unneeded_compile_commands)   