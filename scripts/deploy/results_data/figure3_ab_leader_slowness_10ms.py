x = [0, 1, 4, 7, 10]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS = {
    0: ThroughputLatency(55642, 9.0483),
    1: ThroughputLatency(47048, 10.6237),
    4: ThroughputLatency(32665, 15.4016),
    7: ThroughputLatency(24479, 20.6294),
    10: ThroughputLatency(20162, 24.9816),
}

HS_2 = {
    0: ThroughputLatency(55470, 7.1089),
    1: ThroughputLatency(47252, 8.4064),
    4: ThroughputLatency(32642, 12.2794),
    7: ThroughputLatency(24938, 16.1366),
    10: ThroughputLatency(20196, 19.9252),
}

HS_1 = {
    0: ThroughputLatency(55303, 5.3013),
    1: ThroughputLatency(47301, 6.2944),
    4: ThroughputLatency(32537, 9.2198),
    7: ThroughputLatency(24916, 12.0650),
    10: ThroughputLatency(20172, 14.9602),
}

HS_1_SLOT = {
    0: ThroughputLatency(55983, 5.161),
    1: ThroughputLatency(54977, 5.2078),
    4: ThroughputLatency(48998, 5.6563),
    7: ThroughputLatency(45032, 5.6839),
    10: ThroughputLatency(39932, 6.1177),
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
