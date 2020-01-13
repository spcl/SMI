'''
	This program generates a dummy topology file. This can be used for testing or skeleton for topology files.
	It takes in input the number of FPGAs, a list of program names, and the output file.
	Programs are associated randomly to FPGAs, and FPGAs are connected in a bus.
'''

import json
import argparse


if __name__ == "__main__":

	parser = argparse.ArgumentParser()
	parser.add_argument("-n", type=int, help='<Required> Number of FPGAs', required=True)
	parser.add_argument('-p', nargs='+', help='<Required> List of programs', required=True)
	parser.add_argument("-f", type=str, help='<Required> Output file', required=True)
	args = vars(parser.parse_args())
	n = args["n"]
	programs = args["p"]
	if n < len(programs):
		print("The number of FPGAs must be greater or equal than the number of programs")
		exit(-1)
	#FPGAs are numbered from 0 to n-1
	programs_to_fpga={}
	for i in range(0,n):
		fpga_name = "fpga-{}:acl0".format(i)
		programs_to_fpga[fpga_name]=programs[i%len(programs)]

   	#create a bus topology: port 0 is connected to port 1 of the next FPGA
	fpga_topology={}
	for i in range(0,n-1):
		src_name = "fpga-{}:acl0:ch0".format(i)
		dst_name = "fpga-{}:acl0:ch1".format(i+1)
		fpga_topology[src_name]=dst_name
	data = {"fpgas": programs_to_fpga, "connections": fpga_topology}
	with open(args["f"], 'w') as f:
		json.dump(data, f, indent=4, separators=(',', ': '))
