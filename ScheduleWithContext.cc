// Core includes
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include <unordered_set>
#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SimpleGossipSimulation");

class GossipApp : public Application {
public:
    GossipApp();
    virtual ~GossipApp();
    void Setup(uint32_t nodeId, Ptr<Node> node, const std::vector<uint32_t>& peers);
    void SendShare(const std::string& shareMsg);
    static std::map<uint32_t, std::vector<uint32_t>> peerList;
    static std::map<uint32_t, std::unordered_set<std::string>> receivedShares;
    static std::map<std::string, std::unordered_set<uint32_t>> shareReceivers;
    static std::map<std::string, std::vector<uint32_t>> shareHopCounts;
    static uint32_t totalUniqueReceives;

private:
    virtual void StartApplication() override;
    virtual void StopApplication() override;

    static void ReceiveShare(uint32_t receiverId, uint32_t senderId, std::string shareMsg, uint32_t hopCount);

    uint32_t m_nodeId;
    Ptr<Node> m_node;
    std::vector<uint32_t> m_peers;

};

class MinerApp : public Application {
public:
    MinerApp();
    virtual ~MinerApp();
    void Setup(uint32_t nodeId, Ptr<GossipApp> gossip);

private:
    virtual void StartApplication() override;
    virtual void StopApplication() override;

    void MineShare();

    uint32_t m_nodeId;
    Ptr<GossipApp> m_gossipApp;
    EventId m_miningEvent;
};

std::map<uint32_t, std::vector<uint32_t>> GossipApp::peerList;
std::map<uint32_t, std::unordered_set<std::string>> GossipApp::receivedShares;
std::map<std::string, std::unordered_set<uint32_t>> GossipApp::shareReceivers;
std::map<std::string, std::vector<uint32_t>> GossipApp::shareHopCounts;
uint32_t GossipApp::totalUniqueReceives = 0;

GossipApp::GossipApp() {}
GossipApp::~GossipApp() {}

void GossipApp::Setup(uint32_t nodeId, Ptr<Node> node, const std::vector<uint32_t>& peers) {
    m_nodeId = nodeId;
    m_node = node;
    m_peers = peers;
    peerList[nodeId] = peers;
}

void GossipApp::StartApplication() {}
void GossipApp::StopApplication() {}

void GossipApp::SendShare(const std::string& shareMsg) {
    if (receivedShares[m_nodeId].count(shareMsg)) return;
    receivedShares[m_nodeId].insert(shareMsg);
    shareReceivers[shareMsg].insert(m_nodeId);
    shareHopCounts[shareMsg].push_back(0);
    totalUniqueReceives++;

    for (uint32_t peer : m_peers) {
        Simulator::ScheduleWithContext(
            peer,
            Seconds(0.05 + ((double)(rand() % 1000) / 1000.0)),
            &GossipApp::ReceiveShare,
            peer,
            m_nodeId,
            shareMsg,
            1
        );
    }
}

void GossipApp::ReceiveShare(uint32_t receiverId, uint32_t senderId, std::string shareMsg, uint32_t hopCount) {
    if (receivedShares[receiverId].count(shareMsg)) return;
    receivedShares[receiverId].insert(shareMsg);
    shareReceivers[shareMsg].insert(receiverId);
    shareHopCounts[shareMsg].push_back(hopCount);
    totalUniqueReceives++;
    NS_LOG_INFO("[Receive] Node " << receiverId << " received share from Node " << senderId << " (hop: " << hopCount << "): " << shareMsg);

    for (uint32_t peer : peerList[receiverId]) {
        if (peer != senderId) {
            Simulator::ScheduleWithContext(
                peer,
                Seconds(0.05 + ((double)(rand() % 1000) / 1000.0)),
                &GossipApp::ReceiveShare,
                peer,
                receiverId,
                shareMsg,
                hopCount + 1
            );
        }
    }
}

MinerApp::MinerApp() {}
MinerApp::~MinerApp() {}

void MinerApp::Setup(uint32_t nodeId, Ptr<GossipApp> gossip) {
    m_nodeId = nodeId;
    m_gossipApp = gossip;
}

void MinerApp::StartApplication() {
    double startDelay = rand() % 10;
    m_miningEvent = Simulator::Schedule(Seconds(startDelay), &MinerApp::MineShare, this);
}

void MinerApp::StopApplication() {
    Simulator::Cancel(m_miningEvent);
}

void MinerApp::MineShare() {
    std::string share = "Share_" + std::to_string(m_nodeId) + "_" + std::to_string(Simulator::Now().GetSeconds());
    m_gossipApp->SendShare(share);

    double nextTime = 10 + (rand() % 5);
    m_miningEvent = Simulator::Schedule(Seconds(nextTime), &MinerApp::MineShare, this);
}

int main(int argc, char *argv[]) {
    uint32_t numNodes = 5000;
    uint32_t numPeers = 8;
    double stopTime = 20.0;

    CommandLine cmd;
    cmd.AddValue("nodes", "Number of nodes", numNodes);
    cmd.Parse(argc, argv);

    NodeContainer nodes;
    nodes.Create(numNodes);

    std::vector<Ptr<GossipApp>> gossipApps(numNodes);

    for (uint32_t i = 0; i < numNodes; ++i) {
        std::unordered_set<uint32_t> peers;
        while (peers.size() < numPeers) {
            uint32_t peer = rand() % numNodes;
            if (peer != i) peers.insert(peer);
        }

        Ptr<GossipApp> gossip = CreateObject<GossipApp>();
        gossip->Setup(i, nodes.Get(i), std::vector<uint32_t>(peers.begin(), peers.end()));
        nodes.Get(i)->AddApplication(gossip);
        gossipApps[i] = gossip;

        Ptr<MinerApp> miner = CreateObject<MinerApp>();
        miner->Setup(i, gossip);
        nodes.Get(i)->AddApplication(miner);
    }

    Simulator::Stop(Seconds(stopTime));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "\n==== Simulation Summary ====\n";
    std::cout << "Total unique share receives across all nodes: " << GossipApp::totalUniqueReceives << "\n";
    std::cout << "Shares received by number of nodes:\n";
    for (const auto& [share, receivers] : GossipApp::shareReceivers) {
        std::cout << share << " reached " << receivers.size() << " nodes\n";
    }

    std::cout << "\n==== Propagation Report ====\n";
    uint32_t fullyPropagated = 0, partiallyPropagated = 0;
    for (const auto& [share, receivers] : GossipApp::shareReceivers) {
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

    std::cout << "\n==== Average Hop Report ====\n";
    for (const auto& [share, hops] : GossipApp::shareHopCounts) {
        double total = 0;
        for (uint32_t h : hops) total += h;
        std::cout << share << " average hop count: " << (total / hops.size()) << "\n";
    }

    return 0;
}
