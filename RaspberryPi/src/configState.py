import json
import logger

def dump():
    global config
    logger.log(json.dumps(config, indent=4, sort_keys=True))

def save():
    global config
    jsonString = json.dumps(config, indent=4, sort_keys=True)
    open("../config.iot", "w").write(jsonString)

def read():
    global config
    jsonData=open("../config.iot").read()
    config = json.loads(jsonData)

def init():
    global config
    read()
    dump()
