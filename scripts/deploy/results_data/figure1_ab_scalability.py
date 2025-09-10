x = [4, 16, 32, 64]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS = {
    4: ThroughputLatency(89945, 5.4735),
    16: ThroughputLatency(65173, 7.8041),
    32: ThroughputLatency(55642, 9.0037),
    64: ThroughputLatency(37627, 14.0277),
}

HS_2 = {
    4: ThroughputLatency(90253, 4.3366),
    16: ThroughputLatency(65236, 5.9582),
    32: ThroughputLatency(55470, 7.171),
    64: ThroughputLatency(37479, 11.2806),
}

HS_1 = {
    4: ThroughputLatency(90286, 3.2029),
    16: ThroughputLatency(64815, 4.5172),
    32: ThroughputLatency(55303, 5.5422),
    64: ThroughputLatency(37314, 8.4171),
}

HS_1_SLOT = {
    4: ThroughputLatency(90152, 3.21112),
    16: ThroughputLatency(64928, 4.4728),
    32: ThroughputLatency(56983, 5.336),
    64: ThroughputLatency(37711, 8.4536),
}



print("\pgfplotstableread{")
throughput_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    throughput_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        throughput_string += str(d[key].throughput) + " "
    
# Print the results
print(throughput_string)

print("}\scalabiltyThroughput")

print()

print("\pgfplotstableread{")



latency_string = "n HS HS-2 HS-1 HS-1-SLOT"

for key in x:
    latency_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT]:
        latency_string += str(d[key].latency * 1000) + " "
    
# Print the results
print(latency_string)
print("}\scalabiltyLatency")


print()

print("Copy the data above into data_batching.tex")
