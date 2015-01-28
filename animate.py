import serial
import time
import numpy as np

scale = 1

def goto(v):
    # prepare command
    if len(v) == 4 :
        cmd = ('G%07d%07d%07d%07d~' % (v[0], v[1], v[2], v[3]))
    else :
        cmd = ('G%07d%07d%07d%07d~' % (v[0], v[1], v[0], v[1]))
    print cmd
    
    status = None
    while True:
        ser.flushInput()
        
        # send command
        ser.write(cmd)
        
        # wait for positive response
        while (status == None) :
            status = ser.readline()
        if (len(status) > 0 and status[0]!='!') : # means message received
            break
        
    # parse status line
    print status
    
ser = None
def connect(comPort):
    global ser
    
    try:
        ser.close()
    except:
        pass
    
    ser = serial.Serial(comPort, baudrate=9600)
    ser.setTimeout(1)
        
    print ser.name

def animate(pathfile_filename, comPort):
    global scale
    
    connect(comPort)
    
    #size,path = np.load('r_image_to_print.png.npy')
    size,path = np.load(pathfile_filename)

    H, W = 1000000, 1000000
    h, w = size
    dx, dy = 0, 0
    
    if 1.0*w/h > W/H:
        scale = W/w
    else:
        scale = H/h
    
    for stroke in path:
        s = stroke*scale + [dx, dy];
        for v in s:
            goto(v)
           
    ser.close()