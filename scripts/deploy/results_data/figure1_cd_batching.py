x = [100, 1000, 2000, 5000, 10000]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS = {
    100: ThroughputLatency(55642, 9.0037),
    1000: ThroughputLatency(393056, 20.1699),
    2000: ThroughputLatency(653600, 30.6009),
    5000: ThroughputLatency(1007000, 59.5984),
    10000: ThroughputLatency(998000, 110.4805),
}

HS_2 = {
    100: ThroughputLatency(55470, 7.171),
    1000: ThroughputLatency(397826, 15.82379),
    2000: ThroughputLatency(651938, 25.94823),
    5000: ThroughputLatency(1003582, 47.59186),
    10000: ThroughputLatency(998000, 84.84423),
}

HS_1 = {
    100: ThroughputLatency(55303, 5.5422),
    1000: ThroughputLatency(397826, 11.11878),
    2000: ThroughputLatency(657016, 17.11501),
    5000: ThroughputLatency(1003935, 32.2121),
    10000: ThroughputLatency(998000, 60.077),
}

HS_1_SLOT = {
    100: ThroughputLatency(56983, 5.336),
    1000: ThroughputLatency(394827, 11.3962),
    2000: ThroughputLatency(655849, 17.0732),
    5000: ThroughputLatency(1001097, 31.4161),
    10000: ThroughputLatency(1000000, 61.8673),
}



print("\pgfplotstableread{")
throughput_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    throughput_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        throughput_string += str(d[key].throughput) + " "
    
# Print the results
print(throughput_string)

print("}\\batchingThroughput")

print()

print("\pgfplotstableread{")



latency_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    latency_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        latency_string += str(d[key].latency * 1000) + " "
    
# Print the results
print(latency_string)
print("}\\batchingLatency")


print()

print("Copy the data above into data_scalability.tex")

