import binascii
import sys


def usage():
    print("\nScript Usage\n")
    print("To Check crc:")
    print("    Usage: " + sys.argv[0] + " <file> check\n")
    print("To Update crc:")
    print("    Usage: " + sys.argv[0] + " <file>")


if len(sys.argv) > 3 or len(sys.argv) < 2:
    usage()
    sys.exit(1)
if len(sys.argv) == 3 and sys.argv[2] != "check":
    usage()
    sys.exit(1)

filename = sys.argv[1]
byte_list = []
crc = 0x0

# Reading all the bytes from input file
with open(filename, "rb") as f:
    while True:
        byte = f.read(1)
        if not byte:
            break
        byte_list.append(byte)

# This uses the CRC-CCITT polynomial x16 + x12 + x5 + 1,
# often represented as 0x1021
for i in range(4, 512):
    crc = binascii.crc_hqx(byte_list[i], crc)

# Formatting the calculated CRC
crc_calculated = hex(crc)
if len(crc_calculated) == 5:
    crc_calculated = crc_calculated[:2] + "0" + crc_calculated[2:]

byte_0 = byte_list[0]
byte_1 = byte_list[1]

crc_in = "0x" + byte_0.hex() + byte_1.hex()

# Checking calculated matches with existing one
if len(sys.argv) == 3:
    if crc_calculated == crc_in:
        print(1)
    else:
        print(0)
# Updating the calculated CRC back to the file
else:
    updated_crc = bytes.fromhex(crc_calculated[2:])

    del byte_list[0]
    del byte_list[0]

    byte = b"".join(byte_list)
    byte = updated_crc + byte

    with open(filename, "wb") as f:

        f.write(byte)
