# Allows PlatformIO to upload directly to AsyncElegantOTA
#
# To use:
# - copy this script into the same folder as your platformio.ini
# - set the following for your project in platformio.ini:
#
# extra_scripts = platformio_upload.py
# upload_protocol = custom
# upload_url = <your upload URL>
# 
# An example of an upload URL:
# upload_URL = http://192.168.1.123/update

import requests
import hashlib
Import('env')

try:
    from requests_toolbelt import MultipartEncoder, MultipartEncoderMonitor
    from tqdm import tqdm
except ImportError:
    env.Execute("/usr/bin/python3.12 -m pip install requests_toolbelt")
    env.Execute("/usr/bin/python3.12 -m pip install tqdm")
    from requests_toolbelt import MultipartEncoder, MultipartEncoderMonitor
    from tqdm import tqdm

def on_upload(source, target, env):
    firmware_path = str(source[0])
    # firmware_path = ".pio/build/espwroom32/filesystem.bin"
    upload_url = env.GetProjectOption('upload_url')

    print("upload_url", upload_url);
    print("firmware_path", firmware_path);

    with open(firmware_path, 'rb') as firmware:
        md5 = hashlib.md5(firmware.read()).hexdigest()
        firmware.seek(0)

        encoder = MultipartEncoder(fields={
            'MD5': md5, 
            'firmware': ('firmware', firmware, 'application/octet-stream')}
        )
        if firmware_path.endswith('.bin'):
            print("uploading filesystem");
            # https://github.com/ayushsharma82/AsyncElegantOTA/commit/d93ec069478fa9d4da93298c9e9524449b68bee2#diff-8cfa683e43aefc512e25da25edbee29df8bd323afab0597449c7e5310734ab87
            # https://github.com/ayushsharma82/AsyncElegantOTA/blob/ebfb6f5c90d88ccec95b397099f9a1b784c8ec87/src/AsyncElegantOTA.cpp
            encoder = MultipartEncoder(fields={
                'MD5': md5, 
                'firmware': ('filesystem', firmware, 'application/octet-stream')}
            )
        else:
            print("uploading firmware");

        bar = tqdm(desc='Upload Progress',
              total=encoder.len,
              dynamic_ncols=True,
              unit='B',
              unit_scale=True,
              unit_divisor=1024
              )

        monitor = MultipartEncoderMonitor(encoder, lambda monitor: bar.update(monitor.bytes_read - bar.n))

        response = requests.post(upload_url, data=monitor, headers={'Content-Type': monitor.content_type})
        bar.close()
        print(response,response.text)
            
env.Replace(UPLOADCMD=on_upload)
 