import re, fnmatch, base64, json, cantools
from paho.mqtt.client import Client as MQTTClient
from influxdb import InfluxDBClient

def readBit(data, pos):
	byte = pos // 8
	mask = 1 << (7 - pos % 8)
	if data[byte] & mask:
		return 1
	else:
		return 0

def writeBit(data, pos, val):
	byte = pos // 8
	mask = 1 << (7 - pos % 8)

	if val:
		data[byte] = data[byte] | mask
	else:
		data[byte] = data[byte] & ~mask

def copyBits(dst, dstPos, src, srcPos, count):
	for i in range(0, count):
		dstIndex = dstPos + i
		srcIndex = srcPos + i

		writeBit(dst, dstIndex, readBit(src, srcIndex))

def parseMessage(payload):
	result = {}
	bitPos = 0

	for msg in canDB.messages:
		for signal in msg.signals:

			name = msg.name + "/" + signal.name
			whitelisted = False
			for regex in messageWhitelist:
				if regex.search(name) is not None:
					whitelisted = True
					break

			if not whitelisted:
				continue

			dummy = bytearray(8)
			signalStart = cantools.database.utils.start_bit(signal)
			copyBits(dummy, signalStart, payload, bitPos, signal.length)

			parsed = msg.decode(dummy)
			result[name] = parsed[signal.name]

			bitPos = bitPos + signal.length

	return result



def on_connect(client, userdata, flags, rc):
	print("Connected with result code " + str(rc))

	mqtt.subscribe("+/devices/+/up")

def on_message(client, userdata, msg):
	try:
		data = json.loads(msg.payload.decode("utf8"))
	except Exception as e:
		print(e)
		return

	payload = base64.decodestring(data["payload_raw"].encode("utf8"))
	metadata = data["metadata"]
	fields = parseMessage(payload)
	fields["metadata"] = metadata

	body = [
		{
			"measurement": influxConfig["measurement"],
			"tags": {
				"app": data["app_id"],
				"device": data["dev_id"],
				"deviceEUI": data["hardware_serial"],
			},
			"time": metadata["time"],
			"fields": fields,
		},
	]

	for key in influxConfig["tags"]:
		body[0]["tags"][key] = influxConfig["tags"][key]

	client.write_points(body)



with open("config.json") as fd:
	config = json.load(fd)

ttnConfig = config["thethingsnetwork"]
influxConfig = config["influxdb"]
canDB = cantools.database.load_file(config["can-spec"])

messageWhitelist = config["messages"]
for i in range(0, len(messageWhitelist)):
	messageWhitelist[i] = re.compile(fnmatch.translate(messageWhitelist[i]))

mqtt = MQTTClient()
mqtt.on_connect = on_connect
mqtt.on_message = on_message

mqtt.username_pw_set(ttnConfig["username"], ttnConfig["password"])
mqtt.connect(ttnConfig["host"], ttnConfig["port"], 60)
mqtt.loop_forever()

db = InfluxDBClient(
	influxConfig["host"], influxConfig["port"],
	influxConfig["username"], influxConfig["password"],
	influxConfig["database"]
)
