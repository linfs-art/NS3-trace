import pandas as pd
import numpy as np


Inegress_ports = pd.read_csv ("merge.csv", usecols= ['IngressPort', 'EgressPort'])
all_merge_packet = pd.read_csv ("merge.csv")

# print (all_merge_packet)


forward_ports_pd = Inegress_ports.drop_duplicates(subset=None,keep='first',inplace=False)

forward_ports_np = np.array (forward_ports_pd) - 1
print (forward_ports_np)
forward_matrix = np.zeros ([8 * 8]).reshape (8, 8)
print (forward_matrix)
for point in forward_ports_np:
	print (point)
	forward_matrix[point[0], point[1]] = 1
print (forward_matrix)

for i in range (1, 9):
	for j in range (1, 9):
		Fij_key = "f" + str (i) + str (j)
		Fij_value = forward_matrix[i - 1][j - 1]
		all_merge_packet[Fij_key] = Fij_value



groups = all_merge_packet.groupby(['Tag'])

couter = 0
for group in groups:
	couter = couter + 1
	filename = "trace" + str (couter)
	group_ = group[1].drop (['Tag'], axis=1)
	group_.to_csv('./' + filename + '.csv', index=False)

