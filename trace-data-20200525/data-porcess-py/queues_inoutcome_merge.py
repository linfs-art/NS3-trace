import pandas as pd



income_packets = pd.read_csv ("traffic-schedule-income.csv")
outcome_packets = pd.read_csv ("traffic-schedule-outcome.csv")
queues_packets = pd.read_csv ("traffic-trace-queues.csv")

print ("read traffic-schedule-income.csv, raws: ", income_packets.shape [0], "colums: ", income_packets.shape [1])
print ("read traffic-schedule-outcome.csv, raws: ", outcome_packets.shape [0], "colums: ", outcome_packets.shape [1])
print ("read traffic-trace-queue.csv, raws: ", queues_packets.shape [0], "colums: ", queues_packets.shape [1])


inoutcome_packets = pd.merge (income_packets, outcome_packets, on = ['PacketUid', 'Source',
																	 'Destination', 'PacketSize',
																	 'Protocol'])

print ("inoutcome_packets raws: ", inoutcome_packets.shape [0], "colums: ", inoutcome_packets.shape [1])


all_merge_packet = pd.merge (inoutcome_packets, queues_packets, on = ['PacketUid'])

# print (all_merge_packet)
print ("all_merge_packet raws: ", all_merge_packet.shape [0], "colums: ", all_merge_packet.shape [1])


# print (all_merge_packet)
all_merge_packet['FlowId'] = all_merge_packet['Source'] + all_merge_packet['Destination']



# print (all_merge_packet)
 
FlowId_counter = 0
FlowId = all_merge_packet['FlowId'].unique ()
# print (FlowId)

for id in FlowId:
	# print (id)
	all_merge_packet = all_merge_packet.replace (id, FlowId_counter)
	FlowId_counter = FlowId_counter + 1

# df.drop(columns=['B', 'C'])
all_merge_packet = all_merge_packet.reindex (columns = ['FlowId', 'PacketUid',
													'Protocol', 'PacketSize',
													'IngressPort', 'EgressPort', 
													'Type', 'Queue', 'Quantum',
													'PhyRxEndTime', 'PhyTxEndTime',
													'Source', 'Destination', 
													'w1', 'w2', 'w3', 'w4',
													'w5', 'w6', 'w7', 'w8',
													'LinkRate', 'Delay', 'Tag'])

groups = all_merge_packet.groupby(['Source', 'Destination', 'Protocol'])

flow_list = []

for group in groups:
    df = group[1].sort_values (by = ['PhyRxEndTime'])
    df.reset_index(drop=True, inplace=True)
    df['PacketUid'] = range (len (df))
    flow_list.append (df)
    # df.to_csv('./sp-merge/' + filename + '.csv', index=False)

flow_merge_df = pd.concat (flow_list)
flow_merge_df = flow_merge_df.sort_values (by = ['PhyRxEndTime'])
flow_merge_df.reset_index(drop=True, inplace=True)
flow_merge_df.to_csv ("merge.csv", index = False, sep = ',')
