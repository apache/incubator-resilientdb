x = [0, 10, 11, 20, 21, 31]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS_1 = {
    0: ThroughputLatency(61413, 0.00481437),
    10: ThroughputLatency(38649, 0.00865429),
    11: ThroughputLatency(28236, 0.0116237),
    20: ThroughputLatency(23156, 0.0140165),
    21: ThroughputLatency(23055, 0.014507),
    31: ThroughputLatency(20153, 0.0168918),
}

HS_2 = {
    0: ThroughputLatency(61480, 0.0064456),
    10: ThroughputLatency(38955, 0.0106837),
    11: ThroughputLatency(28058, 0.0147005),
    20: ThroughputLatency(23626, 0.017563),
    21: ThroughputLatency(22683, 0.018449),
    31: ThroughputLatency(19824, 0.021979),
}

HS = {
    0: ThroughputLatency(61434, 0.0081136),
    10: ThroughputLatency(38855, 0.0133586),
    11: ThroughputLatency(28112, 0.018322),
    20: ThroughputLatency(23404, 0.022072),
    21: ThroughputLatency(22650, 0.022905),
    31: ThroughputLatency(19787, 0.0271209),
}

HS_1_SLOT = {
    0: ThroughputLatency(61540, 0.00473137),
    10: ThroughputLatency(43137, 0.007182),
    11: ThroughputLatency(30344, 0.009924),
    20: ThroughputLatency(24337, 0.012425),
    21: ThroughputLatency(23874, 0.0126356),
    31: ThroughputLatency(20655, 0.0143684),
}




print("\pgfplotstableread{")
throughput_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    throughput_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        throughput_string += str(d[key].throughput) + " "
    
# Print the results
print(throughput_string)

print("}\\networkdelayThroughputOne")

print()

print("\pgfplotstableread{")



latency_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    latency_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        latency_string += str(d[key].latency) + " "
    
# Print the results
print(latency_string)
print("}\\networkdelayLatencyOne")


print()

print("Copy the data above into data_network_delay_1ms.tex")
