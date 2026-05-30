import subprocess
import time

tx_periods = [1, 2, 5, 10, 15, 20]
node_counts = [[50, 0], [45, 5], [40, 10], [35, 15], [30, 20], [25, 25], [20, 30], [15, 35], [10, 40], [5, 45], [0, 50]]
pktsizes_b = [400, 1000, 1600, 2200, 2800, 3400, 4000]
seeds = [1, 2, 3, 4, 5]

output_file_name = "../../cumulative_results.csv"
iteration = 0

with open(output_file_name, 'a') as outfile:
    outfile.write(time.asctime())
    outfile.write("\nperiodA, nodesA, sentA, recvA, lossA, periodB, nodesB, sentB, recvB, lossB, seed, pktsizeB\n")

    for tx_period in tx_periods:
        for count_a, count_b in node_counts:
            for pktsize_b in pktsizes_b:
                for seed in seeds:
                    command = f"../../ns3 run \"wes-simulation --appPeriodA={tx_period} --appPeriodB={tx_period}" \
                              f" --nDevicesA={count_a} --nDevicesB={count_b} --packetSizeB={pktsize_b} --seed={seed}\""

                    iteration += 1
                    print(iteration, command)
                    result = subprocess.run(command,capture_output=True,text=True,shell=True)

                    if result.returncode != 0:
                        print(result.stderr)

                    outfile.write(result.stdout)

            outfile.flush()