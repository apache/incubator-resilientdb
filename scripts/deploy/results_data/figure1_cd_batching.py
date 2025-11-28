x = [100, 1000, 2000, 5000, 10000]

class ThroughputLatency:
    def __init__(self, throughput, latency):
        self.throughput = throughput
        self.latency = latency

HS = {
#    100: ThroughputLatency(55642, 9.0037),
    100: ThroughputLatency(0, 0),
#    1000: ThroughputLatency(393056, 20.1699),
    1000: ThroughputLatency(0, 0),
#    2000: ThroughputLatency(653600, 30.6009),
    2000: ThroughputLatency(0, 0),
#    5000: ThroughputLatency(1007000, 59.5984),
    5000: ThroughputLatency(0, 0),
#    10000: ThroughputLatency(998000, 110.4805),
    10000: ThroughputLatency(0, 0),
}

HS_2 = {
#    100: ThroughputLatency(55470, 7.171),
    100: ThroughputLatency(44351.57894736842, 0.0069697249999999995),
#    1000: ThroughputLatency(397826, 15.82379),
    1000: ThroughputLatency(140981.05263157896, 0.024751925),
#    2000: ThroughputLatency(651938, 25.94823),
    2000: ThroughputLatency(179751.57894736843, 0.041366575),
#    5000: ThroughputLatency(1003582, 47.59186),
    5000: ThroughputLatency(187052.63157894736, 0.09664127499999998),
#    10000: ThroughputLatency(998000, 84.84423),
    10000: ThroughputLatency(210357.8947368421, 0.162620325),
}

HS_1 = {
#    100: ThroughputLatency(55303, 5.5422),
    100: ThroughputLatency(57232.42105263158, 0.0059790875),
#    1000: ThroughputLatency(397826, 11.11878),
    1000: ThroughputLatency(227029.47368421053, 0.013924875),
#   2000: ThroughputLatency(657016, 17.11501) ,
    2000: ThroughputLatency(316471.5789473684, 0.02088595),
#    5000: ThroughputLatency(1003935, 32.2121),
    5000: ThroughputLatency(339252.63157894736, 0.05006580000000001),
#    10000: ThroughputLatency(998000, 60.077),
    10000: ThroughputLatency(310968.4210526316, 0.09185317500000001),
}

HS_1_SLOT = {
#    100: ThroughputLatency(56983, 5.336),
    100: ThroughputLatency(59397.708333333336, 0.0051389174999999995),
#    1000: ThroughputLatency(394827, 11.3962),
    1000: ThroughputLatency(146475.0, 0.086021625),
#    2000: ThroughputLatency(655849, 17.0732),
    2000: ThroughputLatency(198129.16666666666, 0.04868),
#    5000: ThroughputLatency(1001097, 31.4161),
    5000: ThroughputLatency(253000.0, 0.081467025),
#    10000: ThroughputLatency(1000000, 61.8673),
    10000: ThroughputLatency(280437.5, 0.108040925),
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

