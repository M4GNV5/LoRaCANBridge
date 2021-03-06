import json, fnmatch, re, cantools

def matcher(x):
	return re.compile(fnmatch.translate(x))

with open("config.json") as fd:
	config = json.load(fd)

messageWhitelist = []
for key in config["messages"]:
	messageWhitelist.append(re.compile(fnmatch.translate(key)))

def signalFilter(name):
	for regex in messageWhitelist:
		if regex.search(name) is not None:
			return True

db = cantools.database.load_file(config["can-spec"])

extractions = []
ids = []
bitLen = 0
for msg in db.messages:
	for signal in msg.signals:
		name = msg.name + "/" + signal.name
		if signalFilter(name):
			ids.append(msg.frame_id)

			start = cantools.database.utils.start_bit(signal)

			if len(extractions) > 0 \
				and extractions[-1]["frame_id"] == msg.frame_id \
				and extractions[-1]["start"] + extractions[-1]["length"] == start:
				previous = extractions[-1]
				previous["name"] = previous["name"] + ", " + name
				previous["length"] = previous["length"] + signal.length
			else:
				extractions.append({
					'name': name,
					'frame_id': msg.frame_id,
					'start': cantools.database.utils.start_bit(signal),
					'length': signal.length,
				})

			bitLen = bitLen + signal.length

byteLen = bitLen // 8
if bitLen % 8 != 0:
	byteLen = byteLen + 1

#TODO support more than one LoRa Message
loraMessages = [
	{
		'repetition': config["repetition"],
		'len': byteLen,
		'extractions': extractions
	},
]

extractionsCode = ""
messageCode = "Message messages[] = {\n"

for i, message in enumerate(loraMessages):
	extractionsName = "message" + str(i) + "Extractions"
	extractionsCode = extractionsCode + "CANExtraction " + extractionsName + "[] = {\n"

	for extraction in extractions:
		extractionsCode = extractionsCode + "\t{\n" + \
				"\t\t// " + extraction['name'] + ",\n" + \
				"\t\t.id = " + hex(extraction['frame_id']) + ",\n" + \
				"\t\t.pos = " + str(extraction['start']) + ",\n" + \
				"\t\t.len = " + str(extraction['length']) + ",\n" + \
			"\t},\n"

	extractionsCode = extractionsCode + "};\n"

	messageCode = messageCode + "\t{\n" + \
			"\t\t.repetition = " + str(message['repetition']) + ",\n" + \
			"\t\t.len = " + str(message['len']) + ",\n" + \
			"\t\t.extractions = " + extractionsName + ",\n" + \
			"\t\t.extractionCount = sizeof(" + extractionsName + ") / sizeof(CANExtraction)\n" + \
		"\t},\n"

messageCode = messageCode + "};\n"

ids = ",\n\t".join(["0x{:x}".format(x) for x in set(ids)])
print("int canIds[] = {\n\t" + ids + "\n};\n")

print(extractionsCode)
print(messageCode)
