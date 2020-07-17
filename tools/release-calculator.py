#!/usr/bin/python3
import argparse
import datetime


parser = argparse.ArgumentParser(description="Get openBMC release version")
parser.add_argument(
    "-d",
    "--date",
    action="store",
    help="Get version released on given date (Optional, is today by default)",
)
parser.add_argument(
    "-p",
    "--patch",
    action="store",
    type=int,
    default=0,
    help="Patch level of version (default: 0)",
)
args = parser.parse_args()

if args.date is not None:
    iso_date = datetime.datetime.strptime(args.date, "%m/%d/%Y").isocalendar()
else:
    iso_date = datetime.date.today().isocalendar()

fmt = "v{}.{:0>2d}.{}".format(iso_date[0], iso_date[1], args.patch)
print(fmt)
