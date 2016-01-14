import subprocess
import os
import sys

def BinaryToHeader (binary, variablename):
    bytesPerLine = 80 // 6
    print ('const unsigned char {} [] = {{'.format (variablename))

    for i in range (0, len (binary), bytesPerLine):
        print ('\t', end='')
        # This will automatically cut off at the end
        bytesForLine = binary [i:i+bytesPerLine]
        for byte in bytesForLine:
            print ('{0:4}, '.format (hex (byte)), end='')
        print ()
    print ('};')

if __name__=='__main__':
    BinaryToHeader (open (sys.argv[1], 'rb').read (), sys.argv[2])
