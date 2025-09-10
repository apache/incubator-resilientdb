x = [0, 1, 4, 7, 10]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS = {
    0: ThroughputLatency(-1000, -1),
    1: ThroughputLatency(-1000, -1),
    4: ThroughputLatency(-1000, -1),
    7: ThroughputLatency(-1000, -1),
    10: ThroughputLatency(-1000, -1),
}

HS_2 = {
    0: ThroughputLatency(-1000, -1),
    1: ThroughputLatency(-1000, -1),
    4: ThroughputLatency(-1000, -1),
    7: ThroughputLatency(-1000, -1),
    10: ThroughputLatency(-1000, -1),
}

HS_1 = {
    0: ThroughputLatency(54547, 5.6013),
    1: ThroughputLatency(52556, 5.8114),
    4: ThroughputLatency(47341, 5.9248),
    7: ThroughputLatency(41321, 6.2697),
    10: ThroughputLatency(33775, 7.608),
}

HS_1_SLOT10 = {
    0: ThroughputLatency(55410, 5.261),
    1: ThroughputLatency(54241, 5.288),
    4: ThroughputLatency(55058, 5.233),
    7: ThroughputLatency(54742, 5.297),
    10: ThroughputLatency(54275, 5.3507),
}

HS_1_SLOT100 = {
    0: ThroughputLatency(55829, 5.42437),
    1: ThroughputLatency(54949, 5.39282),
    4: ThroughputLatency(55392, 5.4476),
    7: ThroughputLatency(55595, 5.43749),
    10: ThroughputLatency(55186, 5.4837),
}





print("\pgfplotstableread{")
throughput_string = "n HS HS-2 HS-1 HS-1-SLOT10 HS-1-SLOT100"

for key in x:
    throughput_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT10, HS_1_SLOT100]:
        throughput_string += str(d[key].throughput) + " "
    
# Print the results
print(throughput_string)

print("}\\rollbackThroughput")

print()

print("\pgfplotstableread{")



latency_string = "n HS HS-2 HS-1 HS-1-SLOT10 HS-1-SLOT100"

for key in x:
    latency_string += "\n" + str(key) + " "
    for d in [HS, HS_2, HS_1, HS_1_SLOT10, HS_1_SLOT100]:
        latency_string += str(d[key].latency * 1000) + " "
    
# Print the results
print(latency_string)
print("}\\rollbackLatency")


print()

print("Copy the data above into data_rollback.tex")
