Import("env", "projenv")

try:
    import configparser
except ImportError:
    import ConfigParser as configparser
from sys import platform
 
# Windows Xtensa GDB doesn't support Python, so we need to use a .gdbinit file only if Linux
#  .gdbinit file adds pretty printers for stdlib types
def update_debug_extra_cmds(source, target, env):   # debug_extra_cmds = source .gdbinit if Linux
    if platform == "linux" or platform == "linux2":           
        project_config = configparser.ConfigParser()
        project_config.read("platformio.ini")
        debug_extra_cmds = project_config.get("embedded", "debug_extra_cmds")+"\n    source .gdbinit"
        project_config.set("embedded", "debug_extra_cmds", debug_extra_cmds)
        print("Updating debug_extra_cmds to include project .gdbinit with pretty printers: "+debug_extra_cmds)
        

# https://docs.platformio.org/en/latest/scripting/actions.html#scripting-actions 
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", update_debug_extra_cmds)    