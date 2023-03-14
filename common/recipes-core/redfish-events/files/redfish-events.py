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
    def __init__(self, sub_host: str, sub_port: int, target_host: str, fru_id: int):
        super().__init__()
        self.subscriber_url = f"http://{sub_host}:{sub_port}"
        self.target_url = f"http://{target_host}"
        self.target_fruid = fru_id
        self.subscriber_host = sub_host
        self.subscriber_port = sub_port
        self.post_buffer = []
        self.task = None
        self.router.add_get("/{tail:.*}", self.get)
        self.router.add_post("/{tail:.*}", self.post)

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

    async def monitorTarget(self):
        logging.info("Monitor thread running")
        lastState = self.isTargetReady()
        time.sleep(2.0)
        while True:
            newState = self.isTargetReady()
            if newState != lastState:
                if newState:
                    logging.info("Target started. Resubscribing for events")
                    self.resubscribe()
                else:
                    logging.info("Target is no longer available")
            lastState = newState
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
        margs = " ".join(event.get("MessageArgs", []))
        log = "CreateTime: " + tsp
        log += " Severity: " + msev
        log += " Message: " + msg
        log += " MessageArgs: " + margs
        # TODO Probably we might want to do this only for certain msev
        logging.info(log)
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
    logPath = config.get("logging", "path", fallback="/var/log/redfish-events.log")
    initLoggers(logPath)
    handler = RedfishEventHandler(host, port, target, fruid)
    handler.run()
