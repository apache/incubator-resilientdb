x = [0, 10, 11, 20, 21, 31]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS_1 = {
    0: ThroughputLatency(61413, 0.00481437),
    10: ThroughputLatency(4279, 0.072883),
    11: ThroughputLatency(840, 0.406511),
    20: ThroughputLatency(751, 0.451494),
    21: ThroughputLatency(738, 0.458069),
    31: ThroughputLatency(656, 0.5132235),
}

HS_2 = {
    0: ThroughputLatency(61480, 0.0064456),
    10: ThroughputLatency(4281, 0.0957783),
    11: ThroughputLatency(839, 0.478674),
    20: ThroughputLatency(751, 0.53998),
    21: ThroughputLatency(738, 0.595957),
    31: ThroughputLatency(656, 0.666422),
}

HS = {
    0: ThroughputLatency(61434, 0.0081136),
    10: ThroughputLatency(4272, 0.119966),
    11: ThroughputLatency(840, 0.59903),
    20: ThroughputLatency(751, 0.676957),
    21: ThroughputLatency(738, 0.735043),
    31: ThroughputLatency(657, 0.8231625),
}

HS_1_SLOT = {
    0: ThroughputLatency(61540, 0.00473137),
    10: ThroughputLatency(21291, 0.0503423),
    11: ThroughputLatency(855, 0.36829),
    20: ThroughputLatency(765, 0.40563),
    21: ThroughputLatency(755, 0.409172),
    31: ThroughputLatency(654, 0.4572),
}


print("\pgfplotstableread{")
throughput_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    throughput_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        throughput_string += str(d[key].throughput) + " "
    
# Print the results
print(throughput_string)

print("}\\networkdelayThroughputFifty")

print()

print("\pgfplotstableread{")



latency_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    latency_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        latency_string += str(d[key].latency) + " "
    
# Print the results
print(latency_string)
print("}\\networkdelayLatencyFifty")


print()

print("Copy the data above into data_network_delay_50ms.tex")
