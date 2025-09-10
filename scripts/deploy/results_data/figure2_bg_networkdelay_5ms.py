x = [0, 10, 11, 20, 21, 31]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS_1 = {
    0: ThroughputLatency(61413, 0.00481437),
    10: ThroughputLatency(26110, 0.012585),
    11: ThroughputLatency(7790, 0.043433),
    20: ThroughputLatency(6786, 0.0493909),
    21: ThroughputLatency(6690, 0.0500853),
    31: ThroughputLatency(5914, 0.057349),
}

HS_2 = {
    0: ThroughputLatency(61480, 0.0064456),
    10: ThroughputLatency(26209, 0.015829),
    11: ThroughputLatency(7778, 0.052099),
    20: ThroughputLatency(6782, 0.060208),
    21: ThroughputLatency(6692, 0.064558),
    31: ThroughputLatency(5913, 0.074019),
}

HS = {
    0: ThroughputLatency(61434, 0.0081136),
    10: ThroughputLatency(26218, 0.019672),
    11: ThroughputLatency(7789, 0.065056),
    20: ThroughputLatency(6784, 0.0749912),
    21: ThroughputLatency(6720, 0.081278),
    31: ThroughputLatency(5917, 0.0910562),
}

HS_1_SLOT = {
    0: ThroughputLatency(61540, 0.00473137),
    10: ThroughputLatency(38044, 0.0107397),
    11: ThroughputLatency(8045, 0.038668),
    20: ThroughputLatency(6865, 0.0449543),
    21: ThroughputLatency(6752, 0.045586),
    31: ThroughputLatency(5940, 0.050812),
}




print("\pgfplotstableread{")
throughput_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    throughput_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        throughput_string += str(d[key].throughput) + " "
    
# Print the results
print(throughput_string)

print("}\\networkdelayThroughputFive")

print()

print("\pgfplotstableread{")



latency_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    latency_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        latency_string += str(d[key].latency) + " "
    
# Print the results
print(latency_string)
print("}\\networkdelayLatencyFive")


print()

print("Copy the data above into data_network_delay_5ms.tex")
