Import("env", "projenv")
import os
import json
from platformio.project.helpers import get_project_dir
import shutil

try:
    import configparser
except ImportError:
    import ConfigParser as configparser
 
def create_json_update_file(source, target, env):   
    try:
        f = open(".env", "r")
        lines = f.readlines()
        for line in lines:
            if line.strip().startswith("#") or line.strip().startswith(";") or line.strip() == "" or "=" not in line:
                continue
            if line.split("=")[0].strip() == "SERVER_OTA_UPDATE_URL":
                url = line.split(" #")[0].split(";")[0].strip().replace('"', "").split("=")[1]
                break
        if not url:
            print("SERVER_OTA_UPDATE_URL not found in .env file, can't build json manifest for the server")
            exit(0)
    except IOError:
        print("File .env not accessible, create one with SERVER_OTA_UPDATE_URL and other variables set")
        exit(0)
    finally:
        f.close()        
        
    project_config = configparser.ConfigParser()
    project_config.read("platformio.ini")
    product_name = project_config.get("metadata", "product_name")
    version = project_config.get("metadata", "release_version")
    build_type = project_config.get("env"+":"+env["PIOENV"], "build_type")
    filesystem = project_config.get("embedded", "board_build.filesystem")
     
    PROJECT_DIR = get_project_dir()
    bins_path = os.path.join(PROJECT_DIR, ".pio", "build", env["PIOENV"])
    
    bin_src_filename = os.path.join(bins_path, "firmware.bin")
    if not os.path.exists(bin_src_filename):
        print("File {} not found".format(bin_src_filename))

    # build json for manifest
    data = {}
    product_type = product_name.replace(" ", "-").lower() + "-" + build_type
    data["type"] = product_type
    data["version"] = version
    filename = product_type + "-v" + version + "-" + build_type
    
    server_path = os.path.dirname(url)
    
    bin_filename = filename + ".bin"
    if os.path.exists(bin_src_filename):
        data["url"] = server_path + "/" + bin_filename

    spiffs_src_filename = os.path.join(bins_path, "spiffs.bin")
    spiffs_filename = filename + ".spiffs.bin" 
    if filesystem and os.path.exists(spiffs_src_filename):
        data["spiffs"] = server_path + "/" + spiffs_filename
        
    # save json manifest        
    file_data = json.dumps(data)

    ota_build_path = os.path.join(PROJECT_DIR, ".pio", "build", "ota", build_type)
    json_filename = os.path.join(ota_build_path, os.path.basename(url))
    os.makedirs(os.path.dirname(json_filename), exist_ok=True)
    file = open(json_filename, "w")
    file.write(file_data)
    file.close()

    print("Created JSON file at {}: {}".format(json_filename, file_data))
            
    if os.path.exists(bin_src_filename):
        bin_dst_filename = os.path.join(ota_build_path, bin_filename)
        shutil.copyfile(bin_src_filename, bin_dst_filename) 
        print("Copied {} to {}".format(bin_src_filename, bin_dst_filename))

    if filesystem and os.path.exists(spiffs_src_filename):
        spiffs_dst_filename = os.path.join(ota_build_path, spiffs_filename)
        shutil.copyfile(spiffs_src_filename, spiffs_dst_filename)     
        print("Copied {} to {}".format(spiffs_src_filename, spiffs_dst_filename)) 
        

# https://docs.platformio.org/en/latest/scripting/actions.html#scripting-actions 
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", create_json_update_file)    
env.AddPostAction("$BUILD_DIR/spiffs.bin", create_json_update_file) 