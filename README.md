# Gossip Protocol Simulation in ns-3

## Overview

This project implements a gossip protocol simulation using TCP sockets in ns-3. The simulation leverages wireless ad hoc networks to create a peer-to-peer communication environment, demonstrating how messages propagate throughout a network using the gossip protocol paradigm.

### Key Features

- Message propagation through wireless ad hoc networks
- TCP socket-based communication
- Simulated gossip protocol behavior
- Network visualization capabilities

## Current Implementation

The current simulation has the following characteristics:

- **Node 0** acts as the main sender of messages (analogous to a miner who has found a share)
- Messages propagate throughout the network using the gossip protocol
- Basic message dissemination is demonstrated over a wireless ad hoc network
- TCP sockets handle reliable communication between nodes

## Future Development

### Enhancing the Application Class

- Extend functionality to allow all nodes to both send and receive messages (shares)
- Implement bidirectional communication capabilities
- Add message prioritization and queuing mechanisms

### Defining the Share Structure

- Design a comprehensive share data structure
- Integrate share validation mechanisms
- Implement share propagation rules

### Adding Mining Behavior

- Introduce layered architecture in nodes to simulate miner-like behavior
- Implement computational work simulation
- Add resource management for realistic mining constraints

### Incorporating DAG (Directed Acyclic Graph)

- Research DAG structures and their applicability in this simulation
- Design DAG-based message propagation algorithms
- Implement and test DAG structure within the ns-3 environment

## Requirements

- ns-3 network simulator (version 3.35 or newer recommended)
- C++ compiler with C++14 support
- Basic knowledge of network simulation concepts

## Installation

1. Install ns-3 following the [official installation guide](https://www.nsnam.org/wiki/Installation)
2. Clone this repository into your ns-3 workspace
   ```bash
   git clone https://github.com/yourusername/gossip-protocol-simulation.git
   ```
3. Copy the simulation files to your ns-3 scratch directory
   ```bash
   cp -r gossip-protocol-simulation/* /path/to/ns-3/scratch/
   ```

## How to Run the Simulation

1. Navigate to your ns-3 directory
   ```bash
   cd /path/to/ns-3
   ```

2. Compile the simulation
   ```bash
   ./waf --run "gossip-protocol-sim"
   ```

3. To enable logging (optional)
   ```bash
   NS_LOG=GossipApplication=level_info ./waf --run "gossip-protocol-sim"
   ```

4. To generate visualization data (optional)
   ```bash
   ./waf --run "gossip-protocol-sim --pcap=1 --animation=1"
   ```

## Simulation Parameters

The simulation can be configured using the following parameters:

| Parameter | Description | Default Value |
|-----------|-------------|---------------|
| `--nodes` | Number of nodes in the simulation | 10 |
| `--time` | Simulation duration in seconds | 100 |
| `--range` | Transmission range in meters | 100 |
| `--packetSize` | Size of packets in bytes | 1024 |
| `--interval` | Message generation interval | 1.0 |

Example with custom parameters:
```bash
./waf --run "gossip-protocol-sim --nodes=20 --time=200 --range=150"
```

## Project Structure

```
gossip-protocol-sim/
├── model/
│   ├── gossip-application.h
│   ├── gossip-application.cc
│   ├── share-structure.h
│   └── share-structure.cc
├── helper/
│   ├── gossip-helper.h
│   └── gossip-helper.cc
└── examples/
    └── gossip-protocol-sim.cc
```

## Visualization

The simulation can generate visualization data that can be viewed using the NetAnim tool provided with ns-3:

1. Generate animation XML file
   ```bash
   ./waf --run "gossip-protocol-sim --animation=1"
   ```

2. Open the file in NetAnim
   ```bash
   ./NetAnim
   ```

3. Load the generated XML file and observe the network behavior

## Troubleshooting

Common issues and their solutions:

- **Compilation errors**: Ensure you're using a compatible version of ns-3
- **Runtime errors**: Check that all dependencies are properly installed
- **No message propagation**: Verify transmission range is sufficient for node density

## Contributing

Contributions to this project are welcome. Please follow these steps:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contact

For questions or support, please open an issue on the project repository or contact the project maintainer.

---

*This README will be updated as the project evolves.*
