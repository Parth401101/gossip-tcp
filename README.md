# Gossip Protocol Simulation in ns-3

## Overview

A simple implementation of gossip protocol where node 0 is main sender of messages that propagate throughout the network.  
You can think of it as node 0 mined a share and now it is propagated throughout the network.
This code is a simulation of a gossip protocol using TCP sockets in ns-3.
it uses Wireless ad hoc networks to simulate a peer-to-peer network.

- **Node 0** acts as the main sender of messages (analogous to a miner who has found a share)
- Messages propagate throughout the network using the gossip protocol
- Basic message dissemination is demonstrated over a wireless ad hoc network
- TCP sockets handle reliable communication between nodes

## My Future Plans / Approach  

- Extend functionality to allow all nodes to both send and receive messages (shares)
- Desgin how shares will look like 
- Introduce more layes to nodes to simulate miner-like behavior
- Introducing latancy for where it is nessesary 

(this part i am not very clear abt) 
- how DAG will be made and how uncles are choosen 

## Installation

1. Install ns-3 following the (https://www.nsnam.org/wiki/Installation](https://www.nsnam.org/releases/ns-3-44/)
2. Run it  ``` ./ns3 run scratch/P2Pool-v2 ```

