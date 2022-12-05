# NS-3 simulator for BFC
This is the NS-3 simulation code for Backpressure Flow Control (NSDI 2022).

## Note
This code can help reproduce figure 10 and 11 from the NSDI paper. The code is not clean and certain parameters are hardcoded. In case you wish you to the change the setup please don't hesitate to contact me (g.pratish@gmail.com).

## Quick Start

### Running the code
./waf --run 'scratch/nsdifinal mix/nsdiconfig.txt' 2>&1 | tee output.log &

### FCT slowdown
python3 calculate_fct_slowdown.py output.log


## Important Files to look at

**point-to-point/model/qbb-net-device.cc** and **.h**:

**network/model/broadcom-node.cc** and **.h**: 

**network/utils/broadcom-egress-queue.cc** and **.h**: 

**applications/model/udp-echo-client.cc**: 

