Import("env")
import os
from platformio.project.helpers import get_project_dir
from urllib.request import urlretrieve

# print(env.Dump())
PROJECT_DIR = get_project_dir()

timezone_csv_url = "https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.csv"
download_path = os.path.join(PROJECT_DIR, ".pio", "build", "timezone_data")
filename = os.path.join(download_path, os.path.basename(timezone_csv_url))
if(os.path.exists(filename)):
    print("File {} already exists, skipping download".format(filename))
else:
    os.makedirs(download_path, exist_ok=True)
    print("Downloading timezones from {} and saving to {}".format(timezone_csv_url, filename))
    urlretrieve(timezone_csv_url, filename)


