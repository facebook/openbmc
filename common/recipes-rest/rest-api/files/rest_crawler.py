#!/usr/bin/env python3
import http.client
import json
import sys


class OpenBMC:
    def __init__(self, host):

        self.host = host
        self.baseurl = host + ":8080"
        self.conn = http.client.HTTPConnection(self.baseurl)
        self.content = dict()
        self.actions = dict()

    def get_node(self, url):
        self.conn.request("GET", url)
        resp = self.conn.getresponse()
        data = resp.read().decode()
        return json.loads(data)

    def do_action(self, url, action):
        headers = {"Content-type": "application/json"}
        data = {"action": action}
        self.conn.request("POST", url, json.dumps(data), headers)
        resp_raw = self.conn.getresponse()
        resp = json.loads(resp_raw.read().decode())
        return resp

    # Make sure to mock out things like sled-cycle and reboot
    # before crawling! :-)
    # easiest way, get rid of execute perms on power-util and reboot
    # commands.
    def crawl(self, url, action_resp_checker=None):
        if url in self.content:
            return
        data = self.get_node(url)
        self.content[url] = data["Information"]
        if len(data["Actions"]) > 0:
            self.actions[url] = data["Actions"]
        for action in data["Actions"]:
            resp = self.do_action(url, action)
            if action_resp_checker is not None:
                action_resp_checker(url, action, resp)
        for res in data["Resources"]:
            self.crawl(url + "/" + res, action_resp_checker)

    def print(self):
        print(self.content)


def read_only_action_checker(url, action, resp):
    expected_resp = {"result": "not-supported"}
    if resp != expected_resp:
        print(
            "ERROR: Unexpected response to action when read-only %s on url: %s"
            % (action, url)
        )
    else:
        print("ACTION %s secured on %s" % (action, url))


def main():
    if len(sys.argv) > 1:
        HOST = sys.argv[1]
    else:
        HOST = "localhost"

    print("Crawling %s" % (HOST))

    obmc = OpenBMC(HOST)
    obmc.crawl("/api", read_only_action_checker)
    # obmc.print()


if __name__ == "__main__":
    main()
