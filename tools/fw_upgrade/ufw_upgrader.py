#!/usr/bin/python3
import argparse
import logging
import os
import sys
import tarfile

from constants import DEFAULT_UPGRADABLE_ENTITIES, ResetMode
from entity_upgrader import FwUpgrader
from fw_json import FwJson


class Main(object):
    tool_help = "JSON Firmware Upgrader"

    def __init__(self):
        self.zipfile = None  # type: str
        self.fw_entity_items = None
        self.check_only = False  # type: bool
        self.list_only = False  # type: bool
        self.reset = ResetMode.NO_RESET  # type: ResetMode
        self.verbose = False  # type: bool
        self.forced_upgrade = False  # type: bool
        self.stop_on_error = True  # type: bool
        self.dryrun = False  # type: bool

    def _fw_arg_parser(self):
        parser = argparse.ArgumentParser(description=self.tool_help)
        parser.add_argument(
            "-z",
            "--zipfile",
            dest="zipfile",
            action="store",
            default=None,
            help="zip file to use, that contains the whole package",
        )
        parser.add_argument(
            "-i",
            "--entities",
            dest="entities",
            action="store",
            default=DEFAULT_UPGRADABLE_ENTITIES,
            help="Comma separated list of entities to upgrade. All, by default",
        )
        parser.add_argument(
            "-r",
            "--reset",
            dest="reset",
            action="store",
            default=ResetMode.NO_RESET.value,
            choices=[
                ResetMode.USERVER_RESET.value,
                ResetMode.HARD_RESET.value,
                ResetMode.NO_RESET.value,
            ],
            help="Need to perform reset or not",
        )
        parser.add_argument(
            "-v",
            "--verbose",
            action="store_true",
            dest="verbose",
            default=False,
            help="Verbose Output",
        )
        parser.add_argument(
            "-s",
            "--stop_on_error",
            action="store_true",
            dest="stop_on_error",
            default=True,
            help="Stop on failure, even when continue_on_error is set on JSON file",
        )
        parser.add_argument(
            "-c",
            "--check_only",
            action="store_true",
            dest="check_only",
            default=False,
            help="Do not upgrade and only check if upgrade needed, "
            "exits with status 2 if upgrade needed",
        )
        parser.add_argument(
            "-l",
            "--list_only",
            action="store_true",
            dest="list_only",
            default=False,
            help="List all supported items in JSON, and exit",
        )
        parser.add_argument(
            "-d",
            "--dryrun",
            action="store_true",
            dest="dryrun",
            default=False,
            help="Dry-run mode. Just printout commands, and not launch upgrade",
        )
        parser.add_argument(
            "-f",
            "--forced",
            action="store_true",
            dest="forced_upgrade",
            default=False,
            help="Force upgrade, even when upgrade condition is not met",
        )

        return parser.parse_args()

    def _validate_args(self, args) -> None:
        if args.zipfile:
            self.zipfile = args.zipfile
            self._unpack_zipfile()
        if args.entities:
            self.fw_entity_items = args.entities.split(",")
        if args.reset:
            self.reset = args.reset
        if args.check_only:
            self.check_only = args.check_only
        if args.dryrun:
            self.dryrun = args.dryrun
        if args.forced_upgrade:
            self.forced_upgrade = args.forced_upgrade
        if args.list_only:
            self.list_only = args.list_only
        if args.stop_on_error:
            self.stop_on_error = args.stop_on_error
        if args.verbose:
            self.verbose = args.verbose

    def _dump_args(self) -> None:
        print("")
        print("=================================================")
        print("===  JSON based firmware upgrader : Settings  ===")
        print("=================================================")
        print(
            "  Zipfile to use          :  " + (self.zipfile if self.zipfile else "None")
        )
        print("  Items to upgrade        :  " + "".join(self.fw_entity_items))
        print("  Check only (no upgrade) :  " + ("yes" if self.check_only else "no"))
        print("  List  only (no upgrade) :  " + ("yes" if self.list_only else "no"))
        print("  Reboot after success    :  " + self.reset)
        print("  Verbose                 :  " + ("yes" if self.verbose else "no"))
        print("  Stop on error           :  " + ("yes" if self.stop_on_error else "no"))
        print("  Dry run mode            :  " + ("yes" if self.dryrun else "no"))
        print("=================================================")
        print("")

    def _unpack_zipfile(self) -> None:
        logging.debug("=== Unzipping file : " + self.zipfile + " ===")
        with tarfile.open(self.zipfile, "r:bz2") as tf:
            tf.extractall(".")
        logging.info("=== Finished unzipping " + self.zipfile + " ===")

    def _set_log_level(self) -> None:
        level = logging.INFO
        if self.verbose:
            level = logging.DEBUG
        logging.basicConfig(stream=sys.stdout, level=level)

    def run(self):
        args = self._fw_arg_parser()
        self._validate_args(args)
        binarypath = os.path.dirname(os.path.abspath(__file__))
        self._set_log_level()
        if self.verbose:
            self._dump_args()

        fw_struct_obj = FwJson(binarypath, self.fw_entity_items)
        ord_json = fw_struct_obj.get_priority_ordered_json()

        if self.list_only:
            fw_struct_obj.print_fw_entity_list(ord_json)
            exit(0)

        fw_upgrader_obj = FwUpgrader(
            ord_json,
            binarypath=binarypath,
            stop_on_error=self.stop_on_error,
            dryrun=self.dryrun,
            forced_upgrade=self.forced_upgrade,
            reset=self.reset,
        )
        if self.check_only:
            needed = fw_upgrader_obj.is_any_upgrade_needed()
            logging.info("Upgrade Needed : {}".format("YES" if needed else "NO"))
            exit(2 if needed else 0)

        # Check version and upgrade one by one
        rc = fw_upgrader_obj.run_upgrade()
        logging.info("=== Finished upgrading firmware")

        # Reboot if needed
        fw_upgrader_obj.reboot_as_needed()

        if not rc:
            logging.info("=== Upgraded with failures")
            exit(1)


if __name__ == "__main__":
    Main().run()
