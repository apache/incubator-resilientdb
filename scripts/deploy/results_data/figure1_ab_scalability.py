x = [4, 16, 32, 64]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS = {
#    4: ThroughputLatency(89945, 5.4735),
    4: ThroughputLatency(94112.72727272728, 0.0050583275),
#    16: ThroughputLatency(65173, 7.8041),
    16: ThroughputLatency(76777.02127659574, 0.0064289125),
#    32: ThroughputLatency(55642, 9.0037),
    32: ThroughputLatency(59135.36842105263, 0.008704050000000001),
#    64: ThroughputLatency(37627, 14.0277),
    64: ThroughputLatency(40829.424083769634, 0.01475465),
}

HS_2 = {
#    4: ThroughputLatency(90253, 4.3366),
    4: ThroughputLatency(99250.90909090909, 0.0038422200000000004),
#    16: ThroughputLatency(65236, 5.9582),
    16: ThroughputLatency(76741.27659574468, 0.0051071675),
#    32: ThroughputLatency(55470, 7.171),
    32: ThroughputLatency(59370.52631578947, 0.00693072),
#    64: ThroughputLatency(37479, 11.2806),
    64: ThroughputLatency(40748.2722513089, 0.011482744999999999),
}

HS_1 = {
#    4: ThroughputLatency(90286, 3.2029),
    4: ThroughputLatency(97541.81818181818, 0.0029669375),
#    16: ThroughputLatency(64815, 4.5172),
    16: ThroughputLatency(76488.08510638298, 0.0038136374999999997),
#    32: ThroughputLatency(55303, 5.5422),
    32: ThroughputLatency(58766.94736842105, 0.00520517),
#    64: ThroughputLatency(37314, 8.4171),
    64: ThroughputLatency(40781.04712041885, 0.00871747),
}

HS_1_SLOT = {
#    4: ThroughputLatency(90152, 3.21112),
    4: ThroughputLatency(105396.66666666667, 0.0026632550000000002),
#    16: ThroughputLatency(64928, 4.4728),
    16: ThroughputLatency(77025.83333333333, 0.0037831774999999997),
#    32: ThroughputLatency(56983, 5.336),
    32: ThroughputLatency(59698.541666666664, 0.0050861),
#    64: ThroughputLatency(37711, 8.4536),
    64: ThroughputLatency(39966.979166666664, 0.0084669775),
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
