import sys
import struct
import os

if len(sys.argv) < 2:
    print "Change Exe Run Mode Application by burlachenkok@gmail.com\nNot sufficient parametrs. 'exe_src_name.exe'"
    sys.exit(-1)

source_file = sys.argv[1]
dest_file = source_file + '_console'

source = open(source_file, "rb")
dest   = open(dest_file, "w+b")
dest.write(source.read())

dest.seek(0x3c)
(PeHeaderOffset,)=struct.unpack("H", dest.read(2))

dest.seek(PeHeaderOffset)
(PeSignature,)=struct.unpack("I", dest.read(4))
if PeSignature != 0x4550:
    print "Error in Find PE header"

dest.seek(PeHeaderOffset + 0x5C)

# console mode
dest.write(struct.pack("H", 0x03))

source.close()
dest.close()

os.remove(source_file)
os.rename(dest_file, source_file)

print "Completed succesfully.."
