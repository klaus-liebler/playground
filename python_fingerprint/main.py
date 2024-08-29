from r503 import R503
from time import sleep

fp = R503(port=10, pw=1234)

print('Num of templates: ', fp.read_valid_template_num())
print(fp.read_index_table())
fp.set_pw()

print('Place your finger on the sensor..')
sleep(1) # Not required to add this line
#print(fp.auto_identify())
print(fp.auto_enroll(2, ))