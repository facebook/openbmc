#!/usr/bin/env python3
"""
Parses S-Records into an Image object. Data in an Image can be referenced by slicing it:

>>> import srec
>>> img = srec.parse("file.S")
>>> first_16_bytes = img[0x0000:0x0010]

Slices always take *byte* addresses even if the image was
parsed with the address_scale option.
"""
import binascii
import pickle
import pickletools


def hx(bs):
    return binascii.b2a_hex(bs).decode("ascii")


def schecksum(bs):
    return (sum(bs, 0) & 0xFF) ^ 0xFF


class Section:
    def __init__(self, offset: int) -> None:
        self.start = offset
        self.data = bytearray()

    @property
    def end(self):
        return self.start + len(self.data)

    def contains(self, addr):
        return addr >= self.start and addr < self.end

    def __getitem__(self, k):
        if not isinstance(k, slice):
            raise Exception("must use a slice")
        return self.data.__getitem__(slice(k.start - self.start, k.stop - self.start))


class SRecord:
    __slots__ = ["stype", "addr", "data"]

    def __init__(self, stype, addr, data):
        self.stype = stype
        self.addr = addr
        self.data = data

    @property
    def end(self):
        return self.addr + len(self.data)


class Image:
    def __init__(self):
        self.sections = []

    def dump(self, path):
        with open(path, "wb") as of:
            for s in self.sections:
                of.seek(s.start)
                of.write(s.data)

    def pickle(self, path):
        with open(path, "wb") as of:
            of.write(pickletools.optimize(pickle.dumps(self)))

    def __getitem__(self, k):
        if not isinstance(k, slice):
            raise Exception("must use slices")
        if k.step is not None:
            raise Exception("cannot step")
        result = bytearray()
        rlen = k.stop - k.start
        while len(result) < rlen:
            pos = k.start + len(result)
            section = next((s for s in self.sections if s.contains(pos)), None)
            if not section:
                next_section = next((s for s in self.sections if s.start > pos), None)
                if next_section:
                    fill = min(next_section.start, k.stop) - pos
                    # print("filling region", fill)
                    result.extend(b"\xff" * fill)
                    continue
                raise Exception(
                    "requested range {:x}, {:x} past end of image".format(pos, k.stop)
                )
            add = section[pos : k.stop]
            result.extend(add)
        return result

    @classmethod
    def from_recs(cls, recs):
        sections = []
        section = None
        for sr in recs:
            if section:
                if sr.addr == section.end:
                    section.data.extend(sr.data)
                    continue
                if sr.addr > section.start and sr.addr < section.end:
                    raise Exception("SRec records overlap", sr)
                else:
                    sections.append(section)
                    # print("{:x} bytes @{:x}".format(len(section.data), section.start))
                    section = None
            section = Section(sr.addr)
            section.data.extend(sr.data)
        if section:
            # print("{:x} bytes @{:x}".format(len(section.data), section.start))
            sections.append(section)
        image = cls()
        image.sections = sections
        return image

    def __str__(self):
        return "Image<{} sections>".format(len(self.sections))


def parse(path, address_scale=1):
    with open(path) as srf:
        for line in srf:
            line = line.strip()
            if line.startswith("S"):
                stype = int(line[1])
                recdata = binascii.a2b_hex(line[2:])
                reclen = recdata[0]
                rec = recdata[1 : 1 + reclen]
                if stype == 0:
                    # Header
                    continue
                if stype in (1, 2, 3):
                    # Data record
                    addrlen = 1 + stype
                    addr = int.from_bytes(rec[0:addrlen], byteorder="big")
                    addr *= address_scale
                    payload = rec[addrlen:-1]
                    checksum = rec[-1]
                    if schecksum(recdata[:-1]) != checksum:
                        raise Exception("Bad SREC Checksum")
                    yield SRecord(stype, addr, payload)
                    continue
                raise Exception("Unhandled record: {}".format(line))
