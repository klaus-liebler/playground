from os import listdir
from os.path import isfile, join
from pathlib import Path

import glob
from PIL import Image
# convert a .png image file to a .bmp image file using PIL


file_in = "./icons/alarm48.png"
img = Image.open(file_in)
new_image = Image.new("RGBA", img.size, "WHITE") # Create a white rgba background
new_image.paste(img, (0, 0), img)
new_image = new_image.convert('1')
new_image = new_image.resize((16,16))

file_out = "test1.bmp"
new_image.save(file_out)