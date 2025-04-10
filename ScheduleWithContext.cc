#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include <unordered_set>
#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SimpleGossipSimulation");

// Global peer list for all nodes
std::map<uint32_t, std::vector<uint32_t>> peerList;
std::map<uint32_t, std::unordered_set<std::string>> receivedShares;
std::map<std::string, std::unordered_set<uint32_t>> shareReceivers;
uint32_t totalUniqueReceives = 0;

// Random latency generator (customize as needed)
double GetRandomLatency() {
    return 0.05 + ((double)(rand() % 1000) / 1000.0); // 50ms to 1s
}

// Receive function
void ReceiveShare(uint32_t receiverId, uint32_t senderId, std::string shareMsg) {
    if (receivedShares[receiverId].count(shareMsg)) {
        return; // Already received
    }
    receivedShares[receiverId].insert(shareMsg);
    shareReceivers[shareMsg].insert(receiverId);
    totalUniqueReceives++;
    NS_LOG_INFO("[Receive] Node " << receiverId << " received share from Node " << senderId << ": " << shareMsg);

    // Forward to peers
    for (uint32_t peer : peerList[receiverId]) {
        if (peer != senderId) {
            Simulator::ScheduleWithContext(
                peer,
                Seconds(GetRandomLatency()),
                &ReceiveShare,
                peer,
                receiverId,
                shareMsg
            );
        }
    }
}

// Miner application that triggers sending a share
void MineShare(Ptr<Node> node) {
    uint32_t nodeId = node->GetId();
    std::string share = "Share_" + std::to_string(nodeId) + "_" + std::to_string(Simulator::Now().GetSeconds());
    receivedShares[nodeId].insert(share);
    shareReceivers[share].insert(nodeId);
    totalUniqueReceives++;

    // Send to peers
    for (uint32_t peer : peerList[nodeId]) {
        Simulator::ScheduleWithContext(
            peer,
            Seconds(GetRandomLatency()),
            &ReceiveShare,
            peer,
            nodeId,
            share
        );
    }

    // Schedule next mining
    double nextTime = 10 + (rand() % 5); // 10–14 sec
    Simulator::Schedule(Seconds(nextTime), &MineShare, node);
}

int main(int argc, char *argv[]) {
    uint32_t numNodes = 1000;
    uint32_t numPeers = 8;
    double stopTime = 60.0;

    CommandLine cmd;
    cmd.AddValue("nodes", "Number of nodes", numNodes);
    cmd.Parse(argc, argv);

    NodeContainer nodes;
    nodes.Create(numNodes);

    // Setup peer lists randomly
    for (uint32_t i = 0; i < numNodes; ++i) {
        std::unordered_set<uint32_t> peers;
        while (peers.size() < numPeers) {
            uint32_t peer = rand() % numNodes;
            if (peer != i) peers.insert(peer);
        }
        peerList[i] = std::vector<uint32_t>(peers.begin(), peers.end());
    }

    // Schedule mining start per node
    for (uint32_t i = 0; i < numNodes; ++i) {
        double startDelay = rand() % 10; // Start within first 10 sec
        Simulator::Schedule(Seconds(startDelay), &MineShare, nodes.Get(i));
    }

    Simulator::Stop(Seconds(stopTime));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "\n==== Simulation Summary ====\n";
    std::cout << "Total unique share receives across all nodes: " << totalUniqueReceives << "\n";
    std::cout << "Shares received by number of nodes:\n";
    for (const auto& [share, receivers] : shareReceivers) {
        std::cout << share << " reached " << receivers.size() << " nodes\n";
    }

    std::cout << "\n==== Propagation Report ====\n";
    uint32_t fullyPropagated = 0, partiallyPropagated = 0;
    for (const auto& [share, receivers] : shareReceivers) {
        if (receivers.size() == numNodes) {
            std::cout << "[FULL ✅] " << share << " reached all " << numNodes << " nodes.\n";
            fullyPropagated++;
        } else {
            std::cout << "[PARTIAL ❌] " << share << " reached only " << receivers.size() << "/" << numNodes << " nodes.\n";
            partiallyPropagated++;
        }
    }
    std::cout << "\nShares fully propagated: " << fullyPropagated << "\n";
    std::cout << "Shares partially propagated: " << partiallyPropagated << "\n";

    return 0;
}
