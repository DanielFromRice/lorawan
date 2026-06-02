import subprocess
import time
import argparse

tx_periods = [2*60, 5*60, 10*60, 15*60, 20*60] # Reported to program in seconds
node_counts = [[50, 0], [40, 10], [30, 20], [20, 30], [10, 40], [0, 50]]
pktsizes_b = [500, 1000, 2000, 3000, 4000]
seeds = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]
data_modes = [-1, 1] # SF7 (auto sf) and SF9

parser = argparse.ArgumentParser()

# Enables manual multiprocessing by batching over node counts
parser.add_argument("-n", "--run_number", type=int, default=0, required=True)
parser.add_argument("-d", "--data_mode", type=int, default=-1, required=False)
args = parser.parse_args()

if args.run_number > len(node_counts) or args.run_number < 0:
    print("Invalid run number")
    exit(1)

if args.data_mode > 3 or args.data_mode < -1:
    print("Invalid data mode")
    exit(1)

output_file_name = "../../cumulative_results_moreseeds_" + str(args.run_number) + ".csv"

iteration = 0

print("Running with run number " + str(args.run_number))

with open(output_file_name, 'a') as outfile:
    outfile.write(time.asctime())
    outfile.write("\nperiodA, nodesA, sentA, recvA, lossA, periodB, nodesB, sentB, recvB, lossB, dataMode, seed, pktsizeB\n")

    for data_mode in data_modes:
        for tx_period in tx_periods:
            for pktsize_b in pktsizes_b:
                for seed in seeds:
                    command = f"../../ns3 run --no-build \"wes-simulation --appPeriodA={tx_period} --appPeriodB={tx_period}" \
                            f" --nDevicesA={node_counts[args.run_number][0]} --nDevicesB={node_counts[args.run_number][1]}"\
                            f" --packetSizeB={pktsize_b} --dataMode={data_mode} --seed={seed}\""

                    iteration += 1
                    print(iteration, command)
                    result = subprocess.run(command,capture_output=True,text=True,shell=True)

                    if result.returncode != 0:
                        print(result.stderr)

                    outfile.write(result.stdout)

                outfile.flush()