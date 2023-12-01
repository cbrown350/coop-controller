
Import("env")

from os.path import isfile
# assert isfile(".env")
# print(env.Dump())
try:
  f = open(".env", "r")
  lines = f.readlines()
  unset = []
  envs = []
  for line in lines:
    if line.strip().startswith("#") or line.strip().startswith(";") or line.strip() == "" or "=" not in line:
      continue
    unset.append("-D{}".format(line.split("=")[0].strip()))
    # envs.append("-D{}".format(line.split("#")[0].strip().replace('"', r'"\"')))
    envs.append("-D"+line.split("#")[0].split(";")[0].strip().replace('"', r'\"'))
  env.ProcessUnFlags(unset)
  print("build flag unset defines: {}".format(unset))
  env.Append(BUILD_FLAGS=envs)
  print("env build_flags defines: {}".format(envs))
except IOError:
  print("File .env not accessible, create one at the root of the project with variables set. See README and platformio.ini.")
  exit(0)
finally:
  f.close()
  
def set_dotenv_ota_values_in_ini(source, target, env):
  # print(env.Dump())  
  try:
    f = open(".env", "r")
    lines = f.readlines()
    envs = [] 
    for line in lines:
      if line.strip().startswith("#") or line.strip().startswith(";") or line.strip() == "" or "=" not in line:
        continue
      if line.split("#")[0].split(";")[0].strip().split("=")[0] == "DEV_OTA_UPDATE_PASSWORD":
        # --auth=DEV_OTA_UPDATE_PASSWORD
        envs.append("--auth="+line.split("#")[0].split(";")[0].strip().split("=")[1])
      break
    env.Append(UPLOAD_FLAGS=envs)
  except IOError:
    print("File .env not accessible, create one at the root of the project with variables set. See README and platformio.ini.")
    exit(0)
  finally:
    f.close()
  
# https://docs.platformio.org/en/latest/scripting/actions.html#scripting-actions  
env.AddPreAction("upload", set_dotenv_ota_values_in_ini)  
env.AddPreAction("uploadfsota", set_dotenv_ota_values_in_ini)  
  