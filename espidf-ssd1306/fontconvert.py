from os import listdir
from os.path import isfile, join
from pathlib import Path

import glob
from PIL import Image
dir_path = r'./icons/*.bmp'
doc="/*\n"

s="constexpr uint8_t icons16x16[]={\n"
symbol_index=1
for file in glob.glob(dir_path):
    with Image.open(file) as im:
        print(file)
        px = im.load()
        s=s+"  "
        for x in range(0, 16):
            s=s+'0b'
            for y in range(7, -1, -1):
                s=s+"0" if px[x, y]>0 else s+"1"
            s=s+", "
        for x in range(0, 16):
            s=s+'0b'
            for y in range(15, 7,-1):
                s=s+"0" if px[x, y]>0 else s+"1"
            s=s+", "
        s=s+"//"+str(Path(file).stem)+":"+str(symbol_index)+"\n"
        doc=doc+str(Path(file).stem)+": \\x"+f'{symbol_index:x}'+"\n"
        symbol_index=symbol_index+1
s=s+"};\n"
doc=doc+"*/\n"

with open("kl_fonts.h", "w") as text_file:
    text_file.write(doc)
    text_file.write(s)