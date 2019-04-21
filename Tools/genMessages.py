import argparse, fnmatch, re, cantools

def matcher(x):
	return re.compile(fnmatch.translate(x))

parser = argparse.ArgumentParser(description="Generate LoRaCANBridge messages.h")
parser.add_argument("--include", default=[], type=matcher, action="append",
	help="Use multiple times to only include the signals provided")
parser.add_argument("--exclude", default=[], type=matcher, action="append",
	help="Use multiple times to include all signals except the ones provided")
parser.add_argument("--repetition", type=int, default=5,
	help="LoRa message interval in minutes")
parser.add_argument("file", nargs=1,
	help=".kcd or .dbc where to load the CAN messages from")
args = parser.parse_args()

db = cantools.database.load_file(args.file[0])

def wildcardMatch(name, matcher):
	for regex in matcher:
		if regex.search(name) is not None:
			return True

	return False
def acceptAny(name):
	return True
def acceptIncluded(name):
	return wildcardMatch(name, args.include)
def acceptNotExcluded(name):
	return not wildcardMatch(name, args.include)

signalFilter = acceptAny
if len(args.include) != 0 and len(args.exclude) != 0:
	print("Cannot specify both --include and --exclude")
	exit(1)
elif len(args.include) != 0:
	signalFilter = acceptIncluded
elif len(args.exclude) != 0:
	signalFilter = acceptNotExcluded

extractions = []
bitLen = 0
for msg in db.messages:
	for signal in msg.signals:
		name = msg.name + "/" + signal.name
		if signalFilter(name):
			extractions.append({
				'name': name,
				'frame_id': msg.frame_id,
				'start': signal.start,
				'length': signal.length,
			})
			bitLen = bitLen + signal.length

byteLen = bitLen // 8
if bitLen % 8 != 0:
	byteLen = byteLen + 1

#TODO support more than one LoRa Message
loraMessages = [
	{
		'repetition': args.repetition,
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
print(extractionsCode)
print(messageCode)