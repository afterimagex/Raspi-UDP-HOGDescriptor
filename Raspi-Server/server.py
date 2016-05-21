import socket
import time
import traceback
import cv2
import cv2.cv as cv
import Image,StringIO
import threading

is_sending=False
cli_address=('',0)

host='127.0.0.1'
port=10220

ser_socket=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
ser_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
ser_socket.bind(("", port))

class UdpReceiver(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.thread_stop=False

    def run(self):
        while not self.thread_stop:
            global cli_address
            global is_sending
            try:
                message,address=ser_socket.recvfrom(2048)
            except:
                traceback.print_exc()
                continue
            cli_address=address
            if message=='startCam':
                print 'start camera'
                is_sending=True
                ser_socket.sendto('startRcv',cli_address)
            if message=='quitCam':
                is_sending=False
                print 'quit camera'
        def stop(self):
            self.thread_stop=True

receiveThread=UdpReceiver()
receiveThread.setDaemon(True)
receiveThread.start()

camera=cv2.VideoCapture(0)
time.sleep(0.5)
camera.set(cv.CV_CAP_PROP_FRAME_WIDTH, 160)
camera.set(cv.CV_CAP_PROP_FRAME_HEIGHT, 240)

#cam=Device()
#cam.setResolution(640,480)

while 1:
    if is_sending:
        (grabbed,img)=camera.read()
        if not grabbed:
            break
        #img=cam.getImage().resize((160,120))
        img=cv2.cvtColor(img,cv2.COLOR_BGR2RGB)
        data=img.tostring()
        ser_socket.sendto(data,cli_address)
        time.sleep(0.05)
    else:
        time.sleep(1)
receiveThread.stop()
ser_socket.close()
