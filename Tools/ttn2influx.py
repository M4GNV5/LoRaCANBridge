import re, fnmatch, base64, json, cantools, requests
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
			signalInfo = None
			for regex, info in messageWhitelist:
				if regex.search(name) is not None:
					signalInfo = info
					break

			if not signalInfo:
				continue

			dummy = bytearray(8)
			signalStart = cantools.database.utils.start_bit(signal)
			copyBits(dummy, signalStart, payload, bitPos, signal.length)

			parsed = msg.decode(dummy)
			value = parsed[signal.name]

			if type(signalInfo) != dict \
				or "invalid-values" not in signalInfo \
				or value not in signalInfo["invalid-values"]:
				result[name] = value

			bitPos = bitPos + signal.length

	return result

def on_connect(client, userdata, flags, rc):
	print("Connected with result code " + str(rc))

	mqtt.subscribe("+/devices/+/up")

def on_message(data): #client, userdata, msg):
	try:
		pass #data = json.loads(msg.payload.decode("utf8"))
	except Exception as e:
		print(e)
		return

	message = data["uplink_message"]
	payload = base64.decodebytes(message["frm_payload"].encode("utf8"))
	fields = parseMessage(payload)

	gateway_lon = 0
	gateway_lat = 0
	gateway_count = 0
	for gateway in message["rx_metadata"]:
		if "location" in gateway:
			lat = gateway["location"]["latitude"]
			lon = gateway["location"]["longitude"]
			gateway_lat += lat
			gateway_lon += lon
			gateway_count += 1

	if gateway_count != 0:
		fields["longitude"] = gateway_lon / gateway_count
		fields["latitude"] = gateway_lat / gateway_count

	fields["metadata"] = json.dumps(message)
	ids = data["end_device_ids"]

	body = [
		{
			"measurement": influxConfig["measurement"],
			"tags": {
				"app": ids["application_ids"]["application_id"],
				"device": ids["device_id"],
				"deviceEUI": ids["dev_eui"],
			},
			"time": data["received_at"],
			"fields": fields,
		},
	]

	for key in influxConfig["tags"]:
		body[0]["tags"][key] = influxConfig["tags"][key]

	try:
		db.write_points(body)
	except Exception as e:
		print(e)



with open("config.json") as fd:
	config = json.load(fd)

ttnConfig = config["thethingsnetwork"]
influxConfig = config["influxdb"]
canDB = cantools.database.load_file(config["can-spec"])

messageWhitelist = []
for key in config["messages"]:
	val = config["messages"][key]
	entry = (re.compile(fnmatch.translate(key)), val)
	messageWhitelist.append(entry)

db = InfluxDBClient(
	influxConfig["host"], influxConfig["port"],
	influxConfig["username"], influxConfig["password"],
	influxConfig["database"]
)

mqtt = MQTTClient()
mqtt.on_connect = on_connect
mqtt.on_message = on_message

if "tls_ca" in ttnConfig:
	mqtt.tls_set(ttnConfig["tls_ca"])
mqtt.username_pw_set(ttnConfig["username"], ttnConfig["password"])
mqtt.connect(ttnConfig["host"], ttnConfig["port"], 60)
mqtt.loop_forever()
