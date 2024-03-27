#!/usr/bin/python3

import asyncio
import configparser
import json
import logging
import logging.config
import sys
import syslog
import time
from typing import Any, Dict, List
from urllib import request

from aiohttp import web


class RedfishEventHandler(web.Application):
    def __init__(
        self,
        sub_host: str,
        sub_port: int,
        target_host: str,
        fru_id: int,
        fru_name: str,
        timeout: int,
    ):
        super().__init__()
        self.subscriber_url = f"http://{sub_host}:{sub_port}/"
        self.target_url = f"http://{target_host}"
        self.target_fruid = fru_id
        self.target_name = fru_name
        self.subscriber_host = sub_host
        self.subscriber_port = sub_port
        self.post_buffer = []
        self.task = None
        self.router.add_get("/{tail:.*}", self.get)
        self.router.add_post("/{tail:.*}", self.post)
        self.lastState = False
        self.lastActive = time.time()
        self.reportedUnreachableSEL = False
        self.maxInactivityTime = timeout

    def sel(self, log: str):
        syslog.syslog(syslog.LOG_CRIT, f"FRU: {self.target_fruid} " + log)

    def isTargetReady(self) -> bool:
        url = self.target_url + "/redfish"
        try:
            req = request.Request(url)
            resp = request.urlopen(req)
            json.loads(resp.read().decode())
        except Exception:
            return False
        return True

    def waitTarget(self):
        while not self.isTargetReady():
            logging.info("Waiting for target...")
            time.sleep(5.0)

    def handleUnreachable(self):
        if self.lastState:
            logging.info("%s Redfish Endpoint is not reachable" % (self.target_name))
            syslog.syslog(
                syslog.LOG_INFO,
                "%s Redfish Endpoint is not reachable" % (self.target_name),
            )
        if (
            not self.reportedUnreachableSEL
            and (time.time() - self.lastActive) >= self.maxInactivityTime
        ):
            self.sel(
                "%s Redfish Unreachable - ASSERT Timed out after %ds"
                % (self.target_name, self.maxInactivityTime)
            )
            self.reportedUnreachableSEL = True

    def handleReachable(self):
        if not self.lastState:
            logging.info(
                "%s Redfish Endpoint is reachable. Resubscribing for events"
                % (self.target_name)
            )
            syslog.syslog(
                syslog.LOG_INFO,
                "%s Redfish Endpoint is reachable. Resubscribing for events"
                % (self.target_name),
            )
            self.resubscribe()
        self.lastActive = time.time()
        if self.reportedUnreachableSEL:
            self.reportedUnreachableSEL = False
            self.sel(
                "%s Redfish Unreachable - DEASSERT Timed out after %ds"
                % (self.target_name, self.maxInactivityTime)
            )

    def handleState(self, state):
        if state:
            self.handleReachable()
        else:
            self.handleUnreachable()
        self.lastState = state

    async def monitorTarget(self):
        logging.info("Monitor thread running")
        self.lastState = self.isTargetReady()
        time.sleep(2.0)
        while True:
            self.handleState(self.isTargetReady())
            await asyncio.sleep(5.0)

    async def startup(self):
        logging.info("Event service started")
        self.resubscribe()
        self.task = asyncio.get_event_loop().create_task(self.monitorTarget())

    def getSubscriptions(self) -> List[str]:
        url = self.target_url + "/redfish/v1/EventService/Subscriptions"
        req = request.Request(url)
        resp = request.urlopen(req)
        obj = json.loads(resp.read().decode())
        ret = []
        for member in obj["Members"]:
            ret.append(member["@odata.id"])
        return ret

    def deleteSubscription(self, url: str):
        logging.info("DELETING:" + url)
        url = self.target_url + url
        req = request.Request(url, method="DELETE")
        request.urlopen(req)

    def subscribe(self):
        odata = {"Destination": self.subscriber_url, "Protocol": "Redfish"}
        data = str(json.dumps(odata)).encode()
        url = self.target_url + "/redfish/v1/EventService/Subscriptions"
        req = request.Request(url, method="POST", data=data)
        resp = request.urlopen(req)
        obj = json.loads(resp.read().decode())
        if len(obj["@Message.ExtendedInfo"]) != 1:
            raise ValueError("More than one message received: " + json.dumps(obj))
        elif obj["@Message.ExtendedInfo"][0]["MessageSeverity"] != "OK":
            raise ValueError("Target rejected subscription request")

    def resubscribe(self):
        try:
            subs = self.getSubscriptions()
            for sub in subs:
                self.deleteSubscription(sub)
            self.subscribe()
            new_subs = self.getSubscriptions()
            logging.info("NEW Subscribers: " + str(new_subs))
        except Exception as e:
            logging.error("Failed to subscribe: " + str(e))
            self.sel("Failed to register for events")

    def handleEvent(self, event: Dict[str, Any]):
        # eid = event.get('EventId', 'Unknown')
        # egid = event.get('EventGroupId', 'Unknown')
        tsp_spec = event.get("EventTimestamp", "Unknown")
        tsp = event.get("EventTimeStamp", tsp_spec)
        sev = event.get("Severity", "Unknown")
        msev = event.get("MessageSeverity", sev)
        msg = event.get("Message", "Unknown")
        mid = event.get("MessageId", "Unknown")
        margs = " ".join(f'"{arg}"' for arg in event.get("MessageArgs", []))
        data = event.get("AdditionalDataURI", None)
        data_size = event.get("AddionalDataSizeBytes", 0)
        log = "CreateTime: " + tsp
        log += " Severity: " + msev
        log += " Message: " + msg
        log += " MessageId: " + mid
        log += " MessageArgs: " + margs
        if data is not None:
            url = self.target_url + data
            data = request.urlopen(request.Request(url)).read()
            data_path = "/mnt/data/" + mid + str(time.time()) + ".dat"
            if len(data) != data_size:
                logging.warning(
                    f"{data_path} is {len(data)} bytes and not expected {data_size}"
                )
            with open(data_path, "wb") as f:
                f.write(data)
            log += " AdditionalData: " + data_path
        logging.info(log)
        # TODO Probably we might want to do this only for certain msev
        self.sel(log)

    def handleMetricReport(self, metrics: Dict[str, Any], name: str):
        log = f"Metric Name: {name}"
        for metric in metrics:
            mid = metric["MetricId"]
            val = metric["MetricValue"]
            tsp = metric["Timestamp"]
            log += f" Id: {mid} Value: {val} Time: {tsp}"
        # TODO is this even a CRIT?
        logging.info(log)
        self.sel(log)

    async def get(self, request: web.Request) -> web.Response:
        try:
            payload = await request.json()
        except Exception:
            logging.error(f"GET[{request.rel_url}]: INVALID JSON")
            return web.json_response({})
        logging.info(f"GET[{request.rel_url}]:" + json.dumps(payload))
        ret = self.post_buffer
        self.post_buffer = []
        return web.json_response(ret)

    async def post(self, request: web.Request) -> web.Response:
        response_obj = {}
        try:
            payload = await request.json()
        except Exception:
            logging.error(f"POST[{request.rel_url}]: INVALID JSON")
            return web.json_response(response_obj)
        log_pre = f"POST[{request.rel_url}] "
        logging.info(log_pre + json.dumps(payload))
        self.post_buffer.append(payload)
        try:
            if "Events" in payload:
                events = payload["Events"]
                if isinstance(events, list):
                    for event in events:
                        self.handleEvent(event)
                elif isinstance(events, dict):
                    self.handleEvent(events)
                else:
                    logging.error(
                        log_pre + "Invalid object in 'Events': " + json.dumps(payload)
                    )
            elif "MetricValues" in payload:
                metrics = payload["MetricValues"]
                name = payload["Name"]
                self.handleMetricReport(metrics, name)
        except Exception as e:
            logging.error(log_pre + "Caught:" + str(e))
        return web.json_response(response_obj)

    def run(self):
        logging.info("Waiting for target")
        self.waitTarget()
        logging.info("Starting event service")
        web.run_app(self, host=self.subscriber_host, port=self.subscriber_port)


def initLoggers(logPath: str):
    syslog.openlog(ident="redfish-events")
    LOGGER_CONF = {
        "version": 1,
        "disable_existing_loggers": False,
        "formatters": {
            "default": {"format": "%(message)s"},
        },
        "handlers": {
            "file": {
                "level": "INFO",
                "formatter": "default",
                "class": "logging.handlers.RotatingFileHandler",
                "filename": logPath,
                "maxBytes": 1048576,
                "backupCount": 3,
                "encoding": "utf8",
            },
            "stdout": {
                "level": "INFO",
                "formatter": "default",
                "class": "logging.StreamHandler",
                "stream": sys.stdout,
            },
        },
        "loggers": {
            "": {
                "handlers": ["file"],
                "level": "DEBUG",
                "propagate": True,
            }
        },
    }
    logging.config.dictConfig(LOGGER_CONF)


if __name__ == "__main__":
    config = configparser.ConfigParser()
    if len(sys.argv) > 1:
        config.read(sys.argv[1])
    else:
        config.read("/etc/redfish-events.cfg")
    host = config.get("subscriber", "hostname")
    port = config.getint("subscriber", "port", fallback=5555)
    target = config.get("target", "hostname")
    fruid = config.getint("target", "fruid")
    timeout = config.getint("target", "max_unreachable_time")
    fruname = config.get("target", "name")
    logPath = config.get("logging", "path", fallback="/var/log/redfish-events.log")
    initLoggers(logPath)
    handler = RedfishEventHandler(host, port, target, fruid, fruname, timeout)
    handler.run()
