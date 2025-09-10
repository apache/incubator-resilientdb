x = [0, 1, 4, 7, 10]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS = {
    0: ThroughputLatency(55114, 9.0483),
    1: ThroughputLatency(53475, 9.3233),
    4: ThroughputLatency(48106, 10.3792),
    7: ThroughputLatency(43448, 11.444),
    10: ThroughputLatency(37906, 13.145),
}

HS_2 = {
    0: ThroughputLatency(55268, 7.1089),
    1: ThroughputLatency(53272, 7.4815),
    4: ThroughputLatency(47367, 8.4208),
    7: ThroughputLatency(43280, 9.1474),
    10: ThroughputLatency(38058, 10.3892),
}

HS_1 = {
    0: ThroughputLatency(55547, 5.3013),
    1: ThroughputLatency(53312, 5.5567),
    4: ThroughputLatency(48249, 6.1187),
    7: ThroughputLatency(43103, 6.8448),
    10: ThroughputLatency(38006, 7.7204),
}

HS_1_SLOT10 = {
    0: ThroughputLatency(56410, 5.161),
    1: ThroughputLatency(56358, 5.1330),
    4: ThroughputLatency(55598, 5.1369),
    7: ThroughputLatency(54981, 5.1432),
    10: ThroughputLatency(54079, 5.1583),
}

HS_1_SLOT100 = {
    0: ThroughputLatency(56483, 5.129),
    1: ThroughputLatency(56622, 5.151),
    4: ThroughputLatency(56266, 5.172),
    7: ThroughputLatency(56033, 5.166),
    10: ThroughputLatency(55691, 5.148),
}




print("\pgfplotstableread{")
throughput_string = "n HS HS-2 HS-1 HS-1-SLOT10 HS-1-SLOT100"

for key in x:
    throughput_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT10, HS_1_SLOT100]:
        throughput_string += str(d[key].throughput) + " "
    
# Print the results
print(throughput_string)

print("}\\tailforkingThroughput")

print()

print("\pgfplotstableread{")



latency_string = "n HS HS-2 HS-1 HS-1-SLOT10 HS-1-SLOT100"

for key in x:
    latency_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT10, HS_1_SLOT100]:
        latency_string += str(d[key].latency * 1000) + " "
    
# Print the results
print(latency_string)
print("}\\tailforkingLatency")


print()

print("Copy the data above into data_tailforking.tex")
