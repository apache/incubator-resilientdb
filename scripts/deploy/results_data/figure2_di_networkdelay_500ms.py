x = [0, 10, 11, 20, 21, 31]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS_1 = {
    0: ThroughputLatency(61413, 0.004814),
    10: ThroughputLatency(447, 0.966892),
    11: ThroughputLatency(83, 4.026693),
    20: ThroughputLatency(73, 4.405486),
    21: ThroughputLatency(72, 4.43314),
    31: ThroughputLatency(65, 4.830668),
}

HS_2 = {
    0: ThroughputLatency(61480, 0.0064456),
    10: ThroughputLatency(446, 1.27789),
    11: ThroughputLatency(84, 4.95771),
    20: ThroughputLatency(74, 5.527367),
    21: ThroughputLatency(74, 5.99258),
    31: ThroughputLatency(66, 6.615701),
}

HS = {
    0: ThroughputLatency(61434, 0.008113),
    10: ThroughputLatency(446, 1.46878),
    11: ThroughputLatency(84, 6.297183),
    20: ThroughputLatency(73, 6.9454675),
    21: ThroughputLatency(73, 7.387172),
    31: ThroughputLatency(66, 8.054873),
}

HS_1_SLOT = {
    0: ThroughputLatency(61540, 0.00473137),
    10: ThroughputLatency(14609, 0.534457),
    11: ThroughputLatency(84, 3.52044),
    20: ThroughputLatency(76, 3.93304),
    21: ThroughputLatency(75, 3.980818),
    31: ThroughputLatency(66, 4.349064),
}




print("\pgfplotstableread{")
throughput_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    throughput_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        throughput_string += str(d[key].throughput) + " "
    
# Print the results
print(throughput_string)

print("}\\networkdelayThroughputFiveHundred")

print()

print("\pgfplotstableread{")



latency_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    latency_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        latency_string += str(d[key].latency) + " "
    
# Print the results
print(latency_string)
print("}\\networkdelayLatencyFiveHundred")


print()

print("Copy the data above into data_network_delay_500ms.tex")
