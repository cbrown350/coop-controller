Import("env")
import os
from platformio.project.helpers import get_project_dir
import shutil

# print(env.Dump())
PROJECT_DIR = get_project_dir()
platform = env.PioPlatform()
FRAMEWORK_DIR = platform.get_package_dir("framework-espidf")
GENERATOR_PY = os.path.join(FRAMEWORK_DIR, "components", "mbedtls", "esp_crt_bundle", "gen_crt_bundle.py")

os.makedirs(os.path.join(PROJECT_DIR, "certs"), exist_ok=True)

from urllib.request import urlretrieve 
cert_urls = [ "https://letsencrypt.org/certs/isrgrootx1.pem", "https://letsencrypt.org/certs/isrg-root-x2.pem", "https://letsencrypt.org/certs/isrg-root-x1-cross-signed.pem" ]
cert_filenames = []
for cert_url in cert_urls:
    filename = os.path.join(PROJECT_DIR, "certs", os.path.basename(cert_url))
    print("Downloading CA cert from {} and saving to {}".format(cert_url, filename))
    urlretrieve(cert_url, filename)
    cert_filenames.append(filename)       
    
# CACERT_ALL = os.path.join(FRAMEWORK_DIR, "components", "mbedtls", "esp_crt_bundle", "cacrt_all.pem")
# cert_filenames.append(CACERT_ALL)
CALOCAL = os.path.join(FRAMEWORK_DIR, "components", "mbedtls", "esp_crt_bundle", "cacrt_local.pem")
cert_filenames.append(CALOCAL)

env.Execute("%s -m pip install cryptography" % (env.get("PYTHONEXE")))
env.Execute("%s %s --input %s" % (env.get("PYTHONEXE"), GENERATOR_PY, " ".join(cert_filenames)))

new_filename = os.path.join(PROJECT_DIR, ".pio", "build", "certs", "ca_certs.bin")
shutil.move(os.path.join(PROJECT_DIR, "x509_crt_bundle"), new_filename)
print("Moved {} to {}".format("x509_crt_bundle", new_filename))


