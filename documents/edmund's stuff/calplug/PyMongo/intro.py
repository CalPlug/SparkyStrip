__author__ = 'edmund'

from pymongo import MongoClient
import datetime


client = MongoClient('mongodb://edmund:silver@ds045107.mongolab.com:45107/calplug_edmund')
db = client['calplug_edmund']
collection = db['power information']
post = {'Device ID':'Edmund',
        'Time':datetime.datetime.utcnow(),
        'Power':'Over 9000',
        'Power Factor':9001,
        'Harmonic Amplitude 1':10,
        'Harmonic Amplitude 2':10.1,
        'Harmonic Amplitude 3':10.2,
        'Harmonic Phase 1':200.99,
        'Harmonic Phase 2':200.98,
        'Harmonic Phase 3':200.97}
posts = db.posts
post_id = posts.insert(post)
#can also insert lists
print('POST ID : ', post_id)
print('COLLECTIONS : ', db.collection_names())
print('find_one() : ', posts.find_one())
print("find_one({'Device ID':'Edmund'}) : ", posts.find_one({'Device ID':'Edmund'}))
client.close()