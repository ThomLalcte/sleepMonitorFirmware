import serial

# open the serial port
espProg = serial.Serial('com17', 115200)

# send the reset command
espProg.write(b'\x11')

# close the serial port
espProg.close()

