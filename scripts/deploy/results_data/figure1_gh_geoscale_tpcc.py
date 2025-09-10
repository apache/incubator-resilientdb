x = [2, 3, 4, 5]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency


HS = {
    2: ThroughputLatency(452, 1.21723),
    3: ThroughputLatency(298, 2.22716),
    4: ThroughputLatency(186, 2.78928),
    5: ThroughputLatency(177, 2.80243),
}

HS_2 = {
    2: ThroughputLatency(451, 0.95258),
    3: ThroughputLatency(291, 1.78365),
    4: ThroughputLatency(187, 2.20764),
    5: ThroughputLatency(178, 2.27834),
}

HS_1 = {
    2: ThroughputLatency(447, 0.72268),
    3: ThroughputLatency(286, 1.28577),
    4: ThroughputLatency(188, 1.68553),
    5: ThroughputLatency(176, 1.77226),
}

HS_1_SLOT = {
    2: ThroughputLatency(444, 0.73389),
    3: ThroughputLatency(286, 1.33495),
    4: ThroughputLatency(188, 1.70249),
    5: ThroughputLatency(172, 1.80982),
}


print("\pgfplotstableread{")
throughput_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    throughput_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        throughput_string += str(d[key].throughput) + " "
    
# Print the results
print(throughput_string)

print("}\geoscaleThroughputTPCC")

print()

print("\pgfplotstableread{")



latency_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    latency_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        latency_string += str(d[key].latency) + " "
    
# Print the results
print(latency_string)
print("}\geoscaleLatencyTPCC")


print()

print("Copy the data above into data_geo_scale_tpcc.tex")


