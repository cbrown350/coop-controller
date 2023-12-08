
from datetime import datetime
Import("env")

# from os.path import isfile
# assert isfile(".env")
# try:
#   f = open(".env", "r")
#   lines = f.readlines()
#   envs = []
#   for line in lines:
#     envs.append("-D{}".format(line.strip()))
#   env.Append(BUILD_FLAGS=envs)
# except IOError:
#   print("File .env not accessible",)
# finally:
#   f.close()

if "debug" in env["PIOENV"] :
    build_num = datetime.now().strftime("%Y%m%d-debug")
else:
    build_num = datetime.now().strftime("%Y%m%d%H%M%S")
print("build_num: {}".format(build_num))
env.Append(BUILD_FLAGS="-DBUILD_NUM={}".format(build_num))
  