x = [0, 1, 4, 7, 10]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS = {
    0: ThroughputLatency(55642, 9.0483),
    1: ThroughputLatency(20264, 24.812),
    4: ThroughputLatency(6962, 72.384),
    7: ThroughputLatency(4189, 124.2736),
    10: ThroughputLatency(3015, 168.3731),
}

HS_2 = {
    0: ThroughputLatency(55470, 7.1089),
    1: ThroughputLatency(20229, 19.8532),
    4: ThroughputLatency(6939, 58.2012),
    7: ThroughputLatency(4132, 98.8459),
    10: ThroughputLatency(3019, 134.4022),
}

HS_1 = {
    0: ThroughputLatency(55303, 5.3013),
    1: ThroughputLatency(20222, 14.8907),
    4: ThroughputLatency(6975, 43.9212),
    7: ThroughputLatency(4211, 72.5262),
    10: ThroughputLatency(3017, 100.9351),
}

HS_1_SLOT = {
    0: ThroughputLatency(55983, 5.129),
    1: ThroughputLatency(53790, 5.4213),
    4: ThroughputLatency(48538, 5.6162),
    7: ThroughputLatency(42923, 5.9481),
    10: ThroughputLatency(36705, 6.5191),
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
