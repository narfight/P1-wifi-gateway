import gzip
import shutil
import os
Import("env")


def compress_firmware(source, target, env):
    """ Compress ESP8266 firmware using gzip for 'compressed OTA upload' """
    SOURCE_FILE = env.subst("$BUILD_DIR") + os.sep + env.subst("$PROGNAME") + ".bin"
    DESTIN_FILE = SOURCE_FILE + ".gz"

    with open(SOURCE_FILE, 'rb') as f_in:
        with gzip.open(DESTIN_FILE, 'wb') as f_out:
            shutil.copyfileobj(f_in, f_out)

    ORG_FIRMWARE_SIZE = os.stat(SOURCE_FILE).st_size
    GZ_FIRMWARE_SIZE = os.stat(DESTIN_FILE).st_size

    print("Compression reduced firmware size by {:.0f}% (was {} bytes, now {} bytes)".format((GZ_FIRMWARE_SIZE / ORG_FIRMWARE_SIZE) * 100, ORG_FIRMWARE_SIZE, GZ_FIRMWARE_SIZE))

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", compress_firmware)
