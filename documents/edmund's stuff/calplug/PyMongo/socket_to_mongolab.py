__author__ = 'edmund'

import socket
import json
import datetime
from pymongo import MongoClient
import struct

MONGO_URI = 'mongodb://edmund:silver@ds045107.mongolab.com:45107/calplug_edmund'
MONGO_DATABASE = 'calplug_edmund'
MONGO_COLLECTION = 'input_testing'
HOST = ''
PORT = 12014
UINT32_MAXVALUE = 4294967295

def parse_socket_data(socket_data) -> (int):
    return struct.unpack('IIIIIIII', socket_data)

def json_encode(encode_data:[int], device_IP) -> json :
    json_data = [{'Device IP': device_IP,
                 'Time': str(datetime.datetime.utcnow()),
                 'Power': encode_data[0],
                 'Power Factor': encode_data[1],
                 'Harmonic Amplitude 1': encode_data[2:5],
                 'Harmonic Phase 1': encode_data[5:]}]
    return json.dumps(json_data)

mongo_connect = MongoClient(MONGO_URI)
mongo_db = mongo_connect[MONGO_DATABASE]
mongo_collection = mongo_db[MONGO_COLLECTION]

dirty_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
dirty_sock.bind((HOST, PORT))
dirty_sock.listen(1)
print('Waiting to accept connection...')
connection, address = dirty_sock.accept()
print('Connected by', address)
while True :
    try :
        raw_data = connection.recv(1024)
        usable_data = parse_socket_data(raw_data)
        if UINT32_MAXVALUE in usable_data :
            break
        json_dict = json.loads(json_encode(usable_data, address))
        print('Inserting data : ', json_dict)
        mongo_collection.insert(json_dict)
    except :
        print('Error thrown, closing connection.')
        connection.close()
print('Closing connection.')
connection.close()