x = [2, 3, 4, 5]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency


HS = {
    2: ThroughputLatency(449, 1.1979),
    3: ThroughputLatency(275, 2.08495),
    4: ThroughputLatency(191, 2.87071),
    5: ThroughputLatency(187, 3.10781),
}

HS_2 = {
    2: ThroughputLatency(453, 0.94802),
    3: ThroughputLatency(270, 1.6589),
    4: ThroughputLatency(193, 2.26848),
    5: ThroughputLatency(186, 2.46017),
}

HS_1 = {
   # 2: ThroughputLatency(449, 0.719953),
    2: ThroughputLatency(103.57894736842105, 3.0429666666666666),
#    3: ThroughputLatency(278, 1.240168),
    3: ThroughputLatency(0, 0),
    4: ThroughputLatency(192, 1.711321),
    4: ThroughputLatency(192, 1.711321),
    5: ThroughputLatency(184, 1.840360),
    5: ThroughputLatency(184, 1.840360),
}

HS_1_SLOT = {
    2: ThroughputLatency(444, 0.720343),
    3: ThroughputLatency(274, 1.24567),
    4: ThroughputLatency(190, 1.7173),
    5: ThroughputLatency(182, 1.847634),
}


print("\pgfplotstableread{")
throughput_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    throughput_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        throughput_string += str(d[key].throughput) + " "
    
# Print the results
print(throughput_string)

print("}\geoscaleThroughputYCSB")

print()

print("\pgfplotstableread{")



latency_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    latency_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        latency_string += str(d[key].latency) + " "
    
# Print the results
print(latency_string)
print("}\geoscaleLatencyYCSB")


print()

print("Copy the data above into data_geo_scale_ycsb.tex")


