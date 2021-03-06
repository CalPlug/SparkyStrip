__author__ = 'edmund'


#save local copies of data and find a way to send IP

import socket
import json
import datetime
from pymongo import MongoClient
import struct

DATA_LENGTH = 10
MONGO_URI = 'mongodb://edmund:silver@ds045107.mongolab.com:45107/calplug_edmund'
MONGO_DATABASE = 'calplug_edmund'
HOST = ''
PORT = 12014

def parse_socket_data(socket_data) -> (int):
    return struct.unpack('f'*DATA_LENGTH, socket_data)

def json_encode(encode_data:[int], device_IP) -> json :

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

def return_int_ip() -> int :
    self_ip = [(s.connect(('8.8.8.8', 80)), s.getsockname()[0], s.close()) for s in [socket.socket(socket.AF_INET, socket.SOCK_DGRAM)]][0][1]
    return struct.unpack("!I", socket.inet_aton(self_ip))[0]

print("IP FOR THIS DEVICE : ", return_int_ip())

mongo_connect = MongoClient(MONGO_URI)
mongo_db = mongo_connect[MONGO_DATABASE]

dirty_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
dirty_sock.bind((HOST, PORT))
dirty_sock.listen(5)
print('Waiting to accept connection...')
connection, address = dirty_sock.accept()
out_file = open('CalPlug_Local_Data', 'a')

mongo_collection = mongo_db[str(address[0])]

print('Connected by', address)
while True :
    try :
        raw_data = connection.recv(1024)
        usable_data = parse_socket_data(raw_data)
        if usable_data == [float(0) for i in range(DATA_LENGTH)] :
            break
        json_dict = json.loads(json_encode(usable_data, address))
        print('Inserting data : ', json_dict)
        mongo_collection.insert(json_dict)
        out_file.write(str(json_dict))
    except Exception as error:
        print('ERROR : ', str(error))
        break
print('Closing connection.')
out_file.close()
connection.close()