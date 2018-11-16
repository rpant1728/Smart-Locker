import time
import serial
import paho.mqtt.client as paho
broker="m15.cloudmqtt.com"
ser = serial.Serial('/dev/ttyACM0', 9600)
#define callback
def on_message(client, userdata, message):
    time.sleep(1)
    s = str(message.payload.decode("utf-8"))
    print("received message =",s)

client= paho.Client()
client.username_pw_set("bpiduhzx", "0TzfC-ryLOxe")
client.on_message=on_message
print("connecting to broker ",broker)
client.connect(broker, 16547)#connect
client.loop_start() #start loop to process received messages
print("subscribing ")
client.subscribe("locker")#subscribe
time.sleep(2)
while True:
	p = str(ser.readline())
	print("publishing ")
	client.publish("locker",p[2:len(p)-3])
	time.sleep(2)
client.loop_stop() #stop loop