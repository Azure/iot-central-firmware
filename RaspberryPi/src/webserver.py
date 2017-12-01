# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. 

from http.server import HTTPServer, SimpleHTTPRequestHandler, BaseHTTPRequestHandler
import socketserver 
import urllib
import json
import configState
import deviceState
import logger

class RequestHandler(BaseHTTPRequestHandler): 

    def log_message(self, format, *args):
        logger.log("==> [{1}] - {0} - {2}\n".format(self.address_string(), self.log_date_time_string(), format%args))

    def set_headers(self, responseCode, contentType):
        self.send_response(responseCode)
        self.send_header("Content-type", contentType)
        if contentType.startswith("text/html"):
            self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        else:
            self.send_header("Cache-Control", "private, max-age=3600")
        self.end_headers()

    def do_HEAD(self):
        if self.path.upper() == "/STATS":
            self.set_headers(200, "text/html; charset=utf-8")
        else:
            self.set_headers(404, "text/html; charset=utf-8")


    def do_GET(self):
        if self.path.upper() == "/START" or self.path.upper() == "/":
            self.set_headers(200, "text/html; charset=utf-8")
            content=open("../content/start.html").read()

            # modify content to reflect config state
            content = content.replace("{{connstr}}", configState.config["hubConnectionString"] if configState.config["hubConnectionString"] != "" else "")
            for key, value in configState.config["sendData"].items():
                content = content.replace("{{" + key + "Check}}", "checked" if value else "")
            content = content.replace("{{interval}}", "5" if configState.config["telemetryInterval"] < 5 else str(configState.config["telemetryInterval"]))
            content = content.replace("{{debug}}", "checked" if configState.config["debug"] else "")
            content = content.replace("{{simulate}}", "checked" if configState.config["simulateSenseHat"] else "")
            content = content.replace("{{showIp}}", "checked" if configState.config["showIp"] else "")

            self.wfile.write(bytes(content, "utf-8"))
        elif self.path.upper() == "/STATS":
            self.set_headers(200, "text/html; charset=utf-8")
            content=open("../content/stats.html").read()

            # replace {{blah-blah-blah}} with values
            content = content.replace("{{deviceName}}", deviceState.getName())
            content = content.replace("{{firmware}}", deviceState.getFirmwareVersion())
            content = content.replace("{{deviceState}}", deviceState.getDeviceState())
            if deviceState.getDeviceState() == "NORMAL":
                color = "green"
            elif deviceState.getDeviceState() == "CAUTION":
                color = "yellow"
            elif deviceState.getDeviceState() == "DANGER":
                color = "red"
            elif deviceState.getDeviceState() == "EXTREME_DANGER":
                color = "blue"
            content = content.replace("{{deviceStateColor}}", color)
            content = content.replace("{{sent}}", str(deviceState.getSentCount()))
            content = content.replace("{{errors}}", str(deviceState.getErrorCount()))
            content = content.replace("{{desired}}", str(deviceState.getDesiredCount()))
            content = content.replace("{{reported}}", str(deviceState.getReportedCount()))
            content = content.replace("{{direct}}", str(deviceState.getDirectCount()))
            content = content.replace("{{c2d}}", str(deviceState.getC2dCount()))
            content = content.replace("{{timestamp}}", str(deviceState.getLastSend()))
            content = content.replace("{{payload}}", deviceState.getLastPayload())
            content = content.replace("{{twin}}", deviceState.getLastTwin())
            content = content.replace("{{interval}}", str(configState.config["telemetryInterval"]*1000))

            self.wfile.write(bytes(content, "utf-8"))
        elif self.path.upper() == "/STYLE.CSS":
            self.set_headers(200, "text/css")
            content=open("../content/style.css").read()
            self.wfile.write(bytes(content, "utf-8"))
        else:
            self.set_headers(404, "text/html; charset=utf-8")


    def updateConfig(self, postBody):
        parts = postBody.split("&")
        for key, value in configState.config["sendData"].items():
            configState.config["sendData"][key] = False
            configState.config["debug"] = False
            configState.config["simulateSenseHat"] = False
            configState.config["showIp"] = False

        for part in parts:
            keyValue = part.split("=")
            keyValue[1] = urllib.parse.unquote(keyValue[1])
            logger.log("{0} : {1}".format(keyValue[0], keyValue[1]))
            if keyValue[0] == "hubConnectionString":
                configState.config["hubConnectionString"] = keyValue[1]
            elif keyValue[0] == "telemetryInterval":
                configState.config["telemetryInterval"] = int(keyValue[1])
            elif keyValue[0] == "debug":
                configState.config["debug"] = True if keyValue[1] == "on" else False
            elif keyValue[0] == "simulate":
                configState.config["simulateSenseHat"] = True if keyValue[1] == "on" else False
            elif keyValue[0] == "showIp":
                configState.config["showIp"] = True if keyValue[1] == "on" else False
            else:
                configState.config["sendData"][keyValue[0]] = True if keyValue[1] == "on" else False
            
        configState.save()
        configState.dump()
            

    def do_POST(self):
        if self.path.upper() == "/RESULT":
            content_len = int(self.headers['content-length'])
            postBody = self.rfile.read(content_len).decode("utf-8")
            self.updateConfig(postBody)
            self.set_headers(200, "text/html; charset=utf-8")
            content=open("../content/complete.html").read()
            self.wfile.write(bytes(content, "utf-8"))


class WebServer:

    def __init__(self, port):
        self.server = HTTPServer(("", port), RequestHandler)
        ipAddress = deviceState.getIPaddress()

        if ipAddress[0] != "127.0.0.1" and ipAddress[0] != "0.0.0.0":
            ip = ipAddress[0]
        elif ipAddress[1] != "127.0.0.1" and ipAddress[1] != "0.0.0.0":
            ip = ipAddress[1]
        else:
            ip = "[unknown]"

        print("Device information being serving at http://{0}:{1}".format(ip, port))
        self.server.serve_forever()

    def shutdown(self):
        self.server.server_close()