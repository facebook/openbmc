#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys, os, argparse
import json, subprocess
import shutil

compatibleStrings = {
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
    "minerva-cmm-cpld" : "com.meta.Hardware.Minerva.CPLD.LCMXO3LF_2100C_cmm",
    "minerva-scm-cpld" : "com.meta.Hardware.Minerva.CPLD.LCMXO3LF_9400HC_scm"
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

def pack_firmware(compatible, version, input_file, output_file):
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    metadata = json.load(open(f"{BASE_DIR}/pldm/metadata-example.json"))
    metadata['PackageHeaderInformation']['PackageVersionString'] = version
    metadata['FirmwareDeviceIdentificationArea'][0]['ComponentImageSetVersionString'] = version
    metadata['ComponentImageInformationArea'][0]['ComponentVersionString'] = version
    metadata['FirmwareDeviceIdentificationArea'][0]['Descriptors'][1]['VendorDefinedDescriptorTitleString'] = compatibleStrings[compatible]
    json.dump(metadata, open(f"{BASE_DIR}/pldm/metadata-example.json", "w"), indent=4)
    cmd = [
        sys.executable, 
        f"{BASE_DIR}/pldm/pldm_fwup_pkg_creator.py",
        f"{BASE_DIR}/pldm/{output_file}", 
        f"{BASE_DIR}/pldm/metadata-example.json",
        input_file,
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    print(result.stderr)
    shutil.move(f"{BASE_DIR}/pldm/{output_file}", f"{BASE_DIR}/{output_file}")
    print("Successfully packed firmware : %s" % os.path.basename(input_file))
    print("Compatible string: %s" % compatible)
    print("Firmware version: %s" % version)
    print("Output file: %s" % output_file)

if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(
        description='LF-OpenBMC firmware packed tool',
        formatter_class=argparse.RawTextHelpFormatter)
    
    parser.add_argument(
        '-c', '--compatible',
        type=str,
        required=True,
        help='Target device compatible string, e.g. %s' % compatible_strings_helper())
    
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
    


