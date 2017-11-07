__author__ = 'edmund'

import json
import datetime
from pymongo import MongoClient

DATA_LENGTH = 10
MONGO_URI = 'mongodb://edmund:silver@ds045107.mongolab.com:45107/calplug_edmund'
MONGO_DATABASE = 'calplug_edmund'

def json_encode(encode_data:[int], device_IP = "NULL") -> json :

    if encode_data[0] == encode_data[1] == 0 :
        json_data = [{'Device IP': device_IP,
                     'Time': str(datetime.datetime.utcnow()),
                     'Type': 'Voltage',
                     'Amplitude': encode_data[2:6],
                     'Phase': encode_data[6:]}]
    else :
        json_data = [{'Device IP': device_IP,
                      'Time': str(datetime.datetime.utcnow()),
                      'Type': 'Current',
                      'Amplitude': encode_data[2:6],
                      'Phase': encode_data[6:],
                      'Power' : encode_data[0],
                      'Power Factor' : encode_data[1]}]
    return json.dumps(json_data)

def array_to_mongo(int_array:[int]) :
    mongo_connect = MongoClient(MONGO_URI)
    mongo_db = mongo_connect[MONGO_DATABASE]
    mongo_collection = mongo_db["LINE INSERTED"]
    json_dict = json.loads(json_encode(int_array))
    mongo_collection.insert(json_dict)
    return