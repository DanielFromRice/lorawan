# Modeling LoRa Data with ns-3
Daniel Rothfusz - UC San Diego WES207, 2026

Masters of Advanced Studies in Wireless Embedded Systems Capstone Project

Big thanks up top here to the authors of the
[LoRaWAN module](https://github.com/signetlabdei/lorawan) I have forked: Davide Magrin,
Martina Capuzzo, Stefano Romagnolo, and Michele Luvisotto. This module was
incredibly helpful and made my task of analyzing a specific situation much easier.
I did not have to develop a tremendous amount of code for this because they already
put a significant amount of useful items in place.

## Overview
This project builds on an existing [ns-3](https://www.nsnam.org "ns-3 Website")
module of a [LoRaWAN](https://www.thethingsnetwork.org/docs/lorawan/what-is-lorawan/)
network to the constraints and expansion opportunities on a specific network.
The main objective is to understand the network present at the
[San Diego Zoo Biodiversity Reserve](https://storymaps.arcgis.com/stories/2e6c74bd38f849e4b021d3c40aa1deba).
Throughout the quarter I worked on this, the driving questions were:
- How can the current network be modeled?
- What is the capacity of the network in this topology?
- Is it possible to insert bursts of audio data into the network?

## Architecture
[ns-3](https://www.nsnam.org "ns-3 Website") is an open source network
simulation utility that is used primarily in academia. The underlying
framework is meant to be a baseline to simulate any variety of network,
and as such has had many modules built up for various network types (WiFi,
LTE, internet routing, etc). It is event-driven, meaning the simulator
increments through program time, handling events as they are placed on
the timeline by objects.

I am using a LoRaWAN module that can be found here:
https://github.com/signetlabdei/lorawan. This module uses the ns-3
framework to create nodes that follow LoRaWAN rules. It has concepts of
"end devices", "gateways", and "network servers" as LoRaWAN specifies. It
is not a complete set of LoRa capability, but it covers enough to create a
large set of networks.

When creating a simulation using this module, a user creates
a set of nodes and then applies properties to them to create the situation.
These properties include things such as the device type, mobility model,
traffic model, and physical layer parameters. Once the properties for all
objects are assigned, the simulation runs and each node automatically schedules
events as needed.

One important reason this simulator was selected is that it simulates both
the MAC and PHY layers. This means it can both schedule realistic traffic in a
programatic way as well as detect collisions by packets sent at the same frequency.
This is an advantage over most other existing LoRa simulators, some of which may
simulate those individual layers better but don't have a way to put them together
in this way.

### Biodiversity Reserve Model
In my simulation, I am simulating two separate groups of nodes. Group A
is meant to represent general monitoring nodes that send very small periodic
updates to the gateway (such as a weather station, which could send 25 byte
payloads). This uses an existing application model called a Periodic Sender, where
each node will send a single packet at a fixed period. Group B represents a
theoretical audio monitoring node, that would periodically attempt to send a
few thousand byte audio payload. I have added a new application model called a
Periodic Burst Sender to emulate this behavior. More details are in [Changeset](#changeset).

The individual nodes within each of the groups are identical, but each group can be
configured separately. Each group can configure:

- Number of nodes within the group
- Payload Size
- Application Period (duration between payloads)

In addition, I have added the option to hardcode the datamode used for all devices.

The nodes are randomly placed in a rectangular region that approximately matches the BDR.
I have also chosen to assume a line-of-sight channel model is sufficient, though
a more advanced channel could be added later.

## Usage
Prerequisites: This simulation requires a C++ compiler and CMake. I
found it simplest to use a WSL environment.

This module can be installed as specified by the original module:
[module-readme](./README_Module.md). This involves installing the main ns-3
program through the [ns-3 installation](https://www.nsnam.org/docs/tutorial/html/quick-start.html)
then adding the LoRaWAN module as a new entry in the src directory. Make sure to
clone this forked repo rather than the original LoRaWAN module. My
simulation has been added as an additional example, titled `wes-simulation`.
Once installed, ns-3 should still be configured using the `--enable_examples` flag.
(`./ns3 configure --enable-examples`)

Once installed and configured, the simulation can be run from the base
directory of ns-3 with: `./ns3 run wes-simulation`. The default behavior
of this command is to rebuild the simulation if changes are detected and
then run with default arguments. The default parameters can be changed
either by modifying the values at the top of the file or by using commandline
values, such as: `./ns3 run "wes-simulation --nDevicesA=30 --nDevicesB=10"`
The quotes are important so ns-3 recognizes the the simulation arguments
as part of one command. If simulations are being run in bulk, the build
check can be disabled by using the `--no-build` flag.

### Bulk test runs
To automate running many runs, I created the script
[iteration-runner.py](./iteration-runner.py). This iterates
through running a set of tx-periods (same for both groups),
node counts, Group B packet size, random seeds, and data modes.
I have set up the script to run for only 1 of the node count values
at a time, specified by the `-n` flag (which selects the node counts
at that index in the list). By creating a separate test for each node
count value, some tests can be run concurrently on different
processes, shortening test time. Note that the iteration runner
includes the `--no-build` flag, so the simulation should be built
manually prior to running the iteration-runner if changes have been
made.

Once all test data is collected from the iteration runner,
data should be aggregated into a single CSV file, just leaving
the header line once at the top. This can then be run through
the [postprocess.py](./postprocess.py) script, which averages the
test data from different seeds and outputs a new csv.

This final CSV file should be placed in the same directory as the
[analysis Jupyter notebook](./analysis.ipynb), which can import
the data and visualize the dataset. This script automatically imports
the different parameter values, and does not need to be modified
beyond the csv filename.

The analysis notebook also contains a python script to dynamically
set parameters and run the script to easily test a range of values.
This behaves identically to setting these parameters through the
commandline and also includes the `--no-build` flag for efficient
data collection.

## Changeset
The following sections detail the specific changes I developed
as a part of my capstone.

### Detailed Packet Tracker
I added additional functionality to [lora-packet-tracker.cc](./helper/lora-packet-tracker.cc) to track individual end devices'
packet counts. This is used in my simulation to separate reported
counts by group, but can also be used to report device success
by even finer resolution, which could be more useful in instances
where fixed location devices with variable spread factors are being
tested.

### US Regional Parameters
I had to modify this simulation to run on US parameters. I started
with a base set of values that were used on an early branch of the
original LoRaWAN module but were not included in the main build.
Those changes are primarily introduced in the
[lorawan-mac-helper](./helper/lorawan-mac-helper.cc).
The module was somewhat built to handle configurable regional
parameter sets, but since that setup was not actually put into
practice there are some gaps. As a result, I ended up also
hardcoding some of these areas to the US parameters for the sake
of my tests, and is a main reason I don't want to contribute these
changes back to the original repository. One such example is for
the [end-device-status](./model/end-device-status.cc), which
hardcodes the gateway reply frequency. Further work would be
required to add this to the general configuration process.

One other important assumption in this area is the number and
type of channels being used. There wasn't an easy way to pass
a configuration for this into the mac-helper, so I have hardcoded
the values to represent the BDR gateway. This means the uplink is
using 16 channels that are all 125kHz. In future tests, particularly
for audio feasibility, it would be worthwhile to compare results
from using some 500kHz channels as well, as allowed within the
US regional parameter set.

### Periodic Burst Packet Sender
LoRaWAN with US parameters does not have a good way to send larger
bursts of data, so for this project I opted to move outside the scope
of the defined end device models. I added what I am calling a Periodic
Burst Sender. In contrast to the Periodic Sender, which sends a single
packet at a fixed interval, this triggers a sequence of packets at that
fixed interval. Each packet will send with a maximum size payload until
the number of bytes selected is exhausted. At that point it will wait the fixed
interval, then repeat.

This sender model is limited by the regional parameters. Because each channel
must wait 20 seconds after sending a maximum size packet, the model
can channel hop through all available channels, then will have to cease transmitting
until the first channel is again available. I did not include the ability to restart
sending after the holdoff time, so the current behavior is to just throw away packets
if there is not a channel available. Therefore, for now the Group B payload size
should be limited to the amount of data that can be sent with max packet size times
the number of channels (one packet on each channel).

This sender model also conflicts with some assumptions in
the simulator of every device having two receive windows (since
the original code only supported
[Class A devices](https://www.thethingsnetwork.org/docs/lorawan/classes/)).
This shouldn't be an issue with tracking end device packets that are received at
the gateway, but if the sim is expanded to track any further functionality then
the behavior should be updated. It should also be considered if this model is implemented
on any real devices.

## Future development
While I am proud of the progress I was able to make during one
quarter, this work is by no means comprehensive and there are
a variety of improvements that would benefit this project.

### Further data collection
I only collected data that was reasonable to gather in a single run on my own computer.
Running a wider range of parameters with properly multiprocessed code on a server with many
more cores would lead to a much more detailed dataset. In particular it would be beneficial
to test with more data modes, differing A/B group times, and other similar variations
that were not tested.

In addition, more work can be done to make representative parameters for the BDR environment.
Selecting as many specific parameters as possible will make it more worthwhile to iterate
over the remaining parameters.

### Improve Burst Model
The periodic burst model has a variety of limitations due to a combination of my short
development time and built-in assumptions of the simulator. For example, I had to set
the inter-packet interval to be 500ms because the system seemed to arbitrarily cause errors
if it was set to 400ms as I believed it capable of. More work should be done to expand the
payload size to be a reasonable payload size for an audio packet to better model performance.

### Evaluate other waveforms
Ultimately LoRa is not as capable at passing larger packet sizes as other waveforms,
so for the zoo's purposes it would also be beneficial to trial other waveforms such
as [WiFi HaLow](https://github.com/imec-idlab/IEEE-802.11ah-ns-3). Note that the HaLow
module is old and uses an older base version of ns-3, so both modules will not be able to
be easily run off the same ns-3 codebase.

## Debugging Tips
ns-3 has a lot of processes going on and the logs can get very complicated to debug very
quickly. Here are some general tips to follow when trying to unravel the program:
- Debug logs are enabled by both file and debug level. I select them at the start of the
simulation file [here](./examples/wes-simulation.cc#L61)
- Try testing the network with 1 end device and a known traffic model to ensure you can
track the behavior
- Note that the reported statistics are packets sent vs received, if packets are aborted
before they are sent they may not be tracked by the packet counter depending on the point
the are aborted.
- The "helper" classes are not necessary for setting up objects and may add restrictions
that are unnecessary. They are useful for tracking behavior when they are able to be used.

