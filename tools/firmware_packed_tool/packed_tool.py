#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys, os, argparse
import json, subprocess
import shutil, urllib.request

pkg_creator_dir = "src"
pkg_creator_tool = "pldm_fwup_pkg_creator.py"
pkg_creator_config = "metadata-example.json"
pkg_commit = "99e8b983ba4fc03dcb2cb89e8c96fce26e77b304"
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

compatibleStrings = {
    "anacapa-bios" : "com.meta.Hardware.Anacapa.SPI.Host",
    "anacapa-scm-cpld" : "com.meta.Hardware.Anacapa.CPLD.LCMXO3D_9400_scm",
    "anacapa-mb-cpld" : "com.meta.Hardware.Anacapa.CPLD.LFMXO5_15D_mb",
    "anacapa-t16-bb-cpld" : "com.meta.Hardware.Anacapa.CPLD.LFMXO5_15D_t16_bb",
    "anacapa-t20-bb-cpld" : "com.meta.Hardware.Anacapa.CPLD.LFMXO5_15D_t20_bb",
    "harma-mb-cpld" : "com.meta.Hardware.Harma.CPLD.LCMXO3LF_4300C_mb",
    "harma-scm-cpld" : "com.meta.Hardware.Harma.CPLD.LCMXO3LF_2100C_scm",
    "harma-aegis-cpld" : "com.meta.Hardware.Harma.CPLD.LCMXO3D_9400HC_aegis",
    "harma-bios" : "com.meta.Hardware.Harma.SPI.Host",
    "harma-isl-vcore0" : "com.meta.Hardware.Harma.VR.ISL69269_vcore0",
    "harma-isl-vcore1" : "com.meta.Hardware.Harma.VR.ISL69269_vcore1",
    "harma-isl-pvdd11" : "com.meta.Hardware.Harma.VR.ISL69269_pvdd11",
    "harma-xdpe-vcore0" : "com.meta.Hardware.Harma.VR.XDPE192C3B_vcore0",
    "harma-xdpe-vcore1" : "com.meta.Hardware.Harma.VR.XDPE192C3B_vcore1",
    "harma-xdpe-pvdd11" : "com.meta.Hardware.Harma.VR.XDPE19283B_pvdd11",
    "harma-retimer" : "com.meta.Hardware.Harma.Retimer.PT5161L",
    "minerva-cmm-cpld" : "com.meta.Hardware.Minerva.CPLD.LCMXO3D_9400HC_cmm",
    "minerva-scm-cpld" : "com.meta.Hardware.Minerva.CPLD.LCMXO3LF_2100C_scm",
    "minerva-fan-board1-cpld" : "com.meta.Hardware.Minerva.CPLD.LFMXO5_25_fanboard1",
    "minerva-fan-board2-cpld" : "com.meta.Hardware.Minerva.CPLD.LFMXO5_25_fanboard2",
    "minerva-fan-board3-cpld" : "com.meta.Hardware.Minerva.CPLD.LFMXO5_25_fanboard3",
    "minerva-fan-board4-cpld" : "com.meta.Hardware.Minerva.CPLD.LFMXO5_25_fanboard4",
    "minerva-fan-board5-cpld" : "com.meta.Hardware.Minerva.CPLD.LFMXO5_25_fanboard5",
    "minerva-fan-board6-cpld" : "com.meta.Hardware.Minerva.CPLD.LFMXO5_25_fanboard6"
}

def compatible_strings_helper():
    helper = "\n"
    for key, value in compatibleStrings.items():
        helper += f"  {key}\n"
    return helper

def compatible_validation(compatible):
    if compatible not in compatibleStrings:
        print(f"Error: Compatible string '{compatible}' is not supported.")
        print("Supported compatible strings are:")
        print(compatible_strings_helper())
        sys.exit(1)

def input_validation():
    if not os.path.isfile(args.input):
        print(f"Error: Input '{args.input}' does not exist or is not a file.")
        sys.exit(1)

def has_access(url, timeout=1):
    # Check if the URL is accessible
    try:
        with urllib.request.urlopen(url, timeout=timeout) as response:
            return response.status == 200
    except Exception:
        print(f"Access check failed: {e}")
        print(f"Error: Unable to access {url}. Please check your internet connection or the URL.")
        sys.exit(1)

def download_file(url, filename, timeout=10):
    print(f"📥 Downloading to {filename} ...")
    try:
        with urllib.request.urlopen(url, timeout=timeout) as response, open(filename, 'wb') as out_file:
            block_size = 8192
            while True:
                chunk = response.read(block_size)
                if not chunk:
                    break
                out_file.write(chunk)
        print(f"✅ Downloaded {filename}")
    except socket.timeout:
        print("❌ Download timed out.")
        sys.exit(1)
    except Exception as e:
        print(f"❌ Failed to download: {e}")
        sys.exit(1)

def tool_init():
    os.makedirs(f"{BASE_DIR}/{pkg_creator_dir}", exist_ok=True)
    tool = f"{BASE_DIR}/{pkg_creator_dir}/{pkg_creator_tool}"
    conf = f"{BASE_DIR}/{pkg_creator_dir}/{pkg_creator_config}"
    
    if not os.path.exists(tool) or not os.path.exists(conf):
        print("Tool not found, downloading...")

        print(f"Downloading {pkg_creator_tool} from GitHub commit {pkg_commit}...")
        url = f"https://raw.githubusercontent.com/openbmc/pldm/{pkg_commit}/tools/fw-update/{pkg_creator_tool}"
        has_access(url)
        download_file(url, tool)

        print(f"Downloading {pkg_creator_config} from GitHub commit {pkg_commit}...")
        url = f"https://raw.githubusercontent.com/openbmc/pldm/{pkg_commit}/tools/fw-update/{pkg_creator_config}"
        has_access(url)
        download_file(url, conf)

def update_config(config, version, compatible):
    metadata = json.load(open(f"{BASE_DIR}/{pkg_creator_dir}/{pkg_creator_config}"))
    del metadata['FirmwareDeviceIdentificationArea'][1:]
    del metadata['ComponentImageInformationArea'][1:]
    metadata['PackageHeaderInformation']['PackageVersionString'] = version
    metadata['FirmwareDeviceIdentificationArea'][0]['ApplicableComponents'] = [0]
    metadata['FirmwareDeviceIdentificationArea'][0]['ComponentImageSetVersionString'] = version
    metadata['ComponentImageInformationArea'][0]['ComponentVersionString'] = version
    descriptors = metadata['FirmwareDeviceIdentificationArea'][0]['Descriptors']
    for descriptor in descriptors:
        if descriptor['DescriptorType'] == 1:
            descriptor['DescriptorData'] = "15A00000"
        if descriptor['DescriptorType'] == 65535:
            descriptor['VendorDefinedDescriptorTitleString'] = compatibleStrings[compatible]
    json.dump(metadata, open(f"{BASE_DIR}/{pkg_creator_dir}/{pkg_creator_config}", "w"), indent=4)

def pack_firmware(compatible, version, input_file, output_file):
    
    update_config(pkg_creator_config, version, compatible)
    cmd = [
        sys.executable, 
        f"{BASE_DIR}/{pkg_creator_dir}/{pkg_creator_tool}",
        f"{BASE_DIR}/{pkg_creator_dir}/{output_file}", 
        f"{BASE_DIR}/{pkg_creator_dir}/{pkg_creator_config}",
        input_file,
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    print(result.stderr)
    shutil.move(f"{BASE_DIR}/{pkg_creator_dir}/{output_file}", f"{BASE_DIR}/{output_file}")
    print("Successfully packed firmware : %s" % os.path.basename(input_file))
    print("Compatible string: %s" % compatible)
    print("Firmware version: %s" % version)
    print("Output file: %s" % output_file)

if __name__ == "__main__":
    tool_init()
    
    parser = argparse.ArgumentParser(
        description='LF-OpenBMC firmware packed tool',
        formatter_class=argparse.RawTextHelpFormatter)
    
    parser.add_argument(
        '-c', '--compatible',
        type=str,
        required=True,
        help='Target device hardware compatible string, e.g. %s' % compatible_strings_helper())
    
    parser.add_argument(
        '-v', '--version',
        type=str,
        required=True,
        help='Firmware version string.')
    
    parser.add_argument(
        '-i', '--input',
        type=str,
        required=True,
        help='Input file name for the firmware files')
    
    parser.add_argument(
        '-o', '--output',
        type=str,
        help='Output file name for the packed firmware, default is <compatible>_<version>.pldm')
    
    args = parser.parse_args()
    compatible_validation(args.compatible)
    input_validation()
    output = args.output if args.output else f"{args.compatible}_{args.version}.pldm"
    pack_firmware(args.compatible, args.version, args.input, output)
    


