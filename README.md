Gossip Protocol Simulation in ns-3

Overview

This project simulates a gossip protocol using TCP sockets in ns-3. It leverages wireless ad hoc networks to create a peer-to-peer communication environment.

Current Implementation

Node 0 acts as the main sender of messages (analogous to a miner who has found a share).

The message propagates throughout the network using the gossip protocol.

The simulation demonstrates basic message dissemination over a wireless ad hoc network.

Future Plans

1. Enhancing the Application Class

Currently, only Node 0 can send messages.

Future versions will allow all nodes to send and receive messages (shares).

2. Defining the Share Structure

Design how a share will look and integrate it into the simulation.

3. Adding Mining Behavior

Introduce layers in nodes to simulate miner-like behavior.

4. Incorporating DAG (Directed Acyclic Graph)

Researching DAG structures and their applicability in this simulation.

Will update the approach once a concrete plan is formed.

How to Run the Simulation

Ensure ns-3 is installed on your system.

Compile the code within the ns-3 environment.

Run the simulation and observe message propagation.
