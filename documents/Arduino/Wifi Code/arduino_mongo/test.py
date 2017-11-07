import serial
import time
#save local copies of data and find a way to send IP
import time
import sys
import datetime

def get_file_name():  # new
    print "new file"
    return datetime.datetime.now().strftime("%Y-%m-%d_%H.%M.%S.txt")

filename = get_file_name()
f = open(filename,'w')
print f

ser = serial.Serial(
   # port='/dev/tty.usbmodem1411',
    port='/dev/ttyACM0',
    baudrate=115200,
    parity=serial.PARITY_ODD,
    stopbits=serial.STOPBITS_TWO,
    bytesize=serial.SEVENBITS
)

#print 'Enter your commands below.\r\nInsert "exit" to leave the application.'
out = ''
lstTime = datetime.datetime.now().minute
while ser.isOpen() :
    time.sleep(0.1)
    minute = datetime.datetime.now().minute
    if (minute<lstTime) | ((minute-lstTime)>5):
	lstTime = minute
	f.close()
	filename = get_file_name()
	f = open(filename,'w')
	print f
    dataLine = ser.readline()
    print dataLine
#    currenttime = datetime.datetime.now().strftime("%H:%M:%S")
   # f.write(currenttime)
#    print currenttime
    f.write(dataLine)

print "Serial is not available"
