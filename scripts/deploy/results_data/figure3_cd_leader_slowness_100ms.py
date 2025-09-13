x = [0, 1, 4, 7, 10]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS = {
    0: ThroughputLatency(55642, 0.0090483),
    1: ThroughputLatency(20264, 0.02545),
    4: ThroughputLatency(6962, 0.072384),
    7: ThroughputLatency(4189, 0.1242736),
    10: ThroughputLatency(3015, 0.1683731),
}

HS_2 = {
    0: ThroughputLatency(55470, 0.0071089),
    1: ThroughputLatency(20229, 0.0198532),
    4: ThroughputLatency(6939, 0.0582012),
    7: ThroughputLatency(4132, 0.0988459),
    10: ThroughputLatency(3019, 0.1344022),
}

HS_1 = {
    0: ThroughputLatency(55303, 0.0053013),
    1: ThroughputLatency(20283, 0.01535),
    4: ThroughputLatency(6975, 0.043912),
    7: ThroughputLatency(4211, 0.072562),
    10: ThroughputLatency(2914, 0.11574),
}

HS_1_SLOT = {
    0: ThroughputLatency(55983, 0.005129),
    1: ThroughputLatency(55617, 0.0049811),
    4: ThroughputLatency(48538, 0.0056162),
    7: ThroughputLatency(42923, 0.0059481),
    10: ThroughputLatency(41763, 0.00573),
}

PlaceHolder = {
    0: ThroughputLatency(-100, -0.1),
    1: ThroughputLatency(-100, -0.1),
    4: ThroughputLatency(-100, -0.1),
    7: ThroughputLatency(-100, -0.1),
    10: ThroughputLatency(-100, -0.1),
}


print("\pgfplotstableread{")
throughput_string = "n HS HS-2 HS-1 HS-1-SLOT PlaceHolder"

for key in x:
    throughput_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT, PlaceHolder]:
        throughput_string += str(d[key].throughput) + " "
    
# Print the results
print(throughput_string)

print("}\leaderslownessThroughputHundred")

print()

print("\pgfplotstableread{")



latency_string = "n HS HS-2 HS-1 HS-1-SLOT PlaceHolder"

for key in x:
    latency_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT, PlaceHolder]:
        latency_string += str(d[key].latency * 1000) + " "
    
# Print the results
print(latency_string)
print("}\leaderslownessLatencyHundred")


print()

print("Copy the data above into data_leader_slowness_100ms.tex")
