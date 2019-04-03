import os

with open('test.txt','r') as file:
	test_data = file.readlines()

tests = {}
test_num = 0
for line_num in range(len(test_data)):
	if '====== Test #' in test_data[line_num]:
