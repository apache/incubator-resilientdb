x = [0, 1, 4, 7, 10]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS = {
    0: ThroughputLatency(55642, 0.009.0483),
    1: ThroughputLatency(47048, 0.010.6237),
    4: ThroughputLatency(32665, 0.015.4016),
    7: ThroughputLatency(24479, 0.020.6294),
    10: ThroughputLatency(20162, 0.024.9816),
}

HS_2 = {
    0: ThroughputLatency(55470, 0.007.1089),
    1: ThroughputLatency(47252, 0.008.4064),
    4: ThroughputLatency(32642, 0.012.2794),
    7: ThroughputLatency(24938, 0.016.1366),
    10: ThroughputLatency(20196, 0.019.9252),
}

HS_1 = {
    0: ThroughputLatency(55303, 0.005.3013),
    1: ThroughputLatency(47301, 0.006.2944),
    4: ThroughputLatency(32537, 0.009.2198),
    7: ThroughputLatency(24916, 0.012.0650),
    10: ThroughputLatency(20172, 0.014.9602),
}

HS_1_SLOT = {
    0: ThroughputLatency(55983, 0.005161),
    1: ThroughputLatency(54977, 0.0052078),
    4: ThroughputLatency(48998, 0.0056563),
    7: ThroughputLatency(45032, 0.0056839),
    10: ThroughputLatency(39932, 0.0061177),
}



print("\pgfplotstableread{")
throughput_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    throughput_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        throughput_string += str(d[key].throughput) + " "
    
# Print the results
print(throughput_string)

print("}\leaderslownessThroughputTen")

print()

print("\pgfplotstableread{")



latency_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    latency_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        latency_string += str(d[key].latency * 1000) + " "
    
# Print the results
print(latency_string)
print("}\leaderslownessLatencyTen")


print()

print("Copy the data above into data_leader_slowness_10ms.tex")
