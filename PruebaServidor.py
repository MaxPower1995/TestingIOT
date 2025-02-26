
# PROGRAMA QUE LEE EL CONTENIDO DEL BROKER ADAFRUIT EMSETEC

import paho.mqtt.client as mqtt



def on_connect(client, userdata, flags, rc):
   print("Conectado - Codigo de resultado: "+str(rc))


# EN ESTA LINEA SE SUBSCRIBE COMO CLIENTE A TODO LO QUE ENVIA EL BROKER

   client.subscribe("EMSETEC/SERIE200/#")
   #client.subscribe("EMSETEC/#")


def on_message(client, userdata, msg):
   print(msg.topic + "" + str(msg.payload))
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message




#produccion
client.connect("143.198.66.52", 1884, 60)

#prueba
#client.connect("143.244.186.22", 1884, 60)

client.username_pw_set("emsetec", "84705200")


client.loop_forever()
