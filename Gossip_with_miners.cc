#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include <unordered_set>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpGossip");

class TcpGossipApp : public Application {
private:
    Ptr<Socket> m_socket;
    std::vector<Ipv4Address> m_neighbors;

    std::unordered_map<Ptr<Socket>, std::string> pendingMessages;
    std::unordered_map<Ptr<Socket>, Ipv4Address> socketToAddress;
    std::unordered_set<Ptr<Socket>> m_connectedSockets;

    std::unordered_set<std::string> receivedMessages;
    std::unordered_set<std::string> forwardedMessages;

    Ipv4Address m_myAddress;
    uint32_t m_nodeId;
    bool m_isSender;

public:
    TcpGossipApp(Ipv4Address myAddress) : m_myAddress(myAddress), m_isSender(false) {}

    void AddNeighbor(Ipv4Address neighbor) {
        if (neighbor != m_myAddress) {
            m_neighbors.push_back(neighbor);
        }
    }

    void PrintNeighbors() const {
        std::cout << "Neighbors of " << m_myAddress << ":" << std::endl;
        for (const auto& neighbor : m_neighbors) {
            std::cout << neighbor << std::endl;
        }
    }
    
    void StartApplication() override {
        m_nodeId = GetNode()->GetId();
        PrintNeighbors();
        m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
        m_socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 8080));
        m_socket->Listen();

        m_socket->SetAcceptCallback(
            MakeCallback(&TcpGossipApp::AcceptConnection, this),
            MakeCallback(&TcpGossipApp::HandleAccept, this)
        );
        m_socket->SetRecvCallback(MakeCallback(&TcpGossipApp::ReceiveMessage, this));
    }

    bool AcceptConnection(Ptr<Socket> socket, const Address &from) {
        return true;
    }

    void HandleAccept(Ptr<Socket> socket, const Address &from) {
        m_connectedSockets.insert(socket);
        socket->SetRecvCallback(MakeCallback(&TcpGossipApp::ReceiveMessage, this));
    }

    void SendMessage(std::string msg) {
        if (receivedMessages.count(msg) > 0) return;

        receivedMessages.insert(msg);
        ForwardMessage(msg);
    }

    void ReceiveMessage(Ptr<Socket> socket) {
        Address from;
        socket->GetPeerName(from);
        Ipv4Address senderAddress = InetSocketAddress::ConvertFrom(from).GetIpv4();
    
        Ptr<Packet> packet = socket->Recv();
        if (!packet || packet->GetSize() == 0) return;
    
        uint32_t size = packet->GetSize();
        std::vector<uint8_t> buffer(size);  
        packet->CopyData(buffer.data(), size);
        std::string msg(buffer.begin(), buffer.end());
    
        uint32_t senderNodeId = ExtractNodeIdFromIpv4(senderAddress);
    
        // NS_LOG_INFO("Node " << m_nodeId << " received message \"" << msg 
        //             << "\" from Node " << senderNodeId << " (" << senderAddress << ")");
    
        if (receivedMessages.count(msg) == 0) {
            receivedMessages.insert(msg);
            Simulator::Schedule(MilliSeconds(10 + rand() % 20), &TcpGossipApp::ForwardMessage, this, msg);
        }
    }
    

    void ForwardMessage(std::string msg) {
        if (forwardedMessages.count(msg) > 0) return;

        forwardedMessages.insert(msg);
        NS_LOG_INFO("Nodef " << m_nodeId << " received message \"" << msg);
        
        for (const auto &neighbor : m_neighbors) {
            Ptr<Socket> sendSocket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
            sendSocket->SetConnectCallback(
                MakeCallback(&TcpGossipApp::HandleConnected, this),
                MakeNullCallback<void, Ptr<Socket>>()
            );
            sendSocket->Connect(InetSocketAddress(neighbor, 8080));
            pendingMessages[sendSocket] = msg;
            socketToAddress[sendSocket] = neighbor;
        }
    }

    void HandleConnected(Ptr<Socket> socket) {
        auto it = pendingMessages.find(socket);
        if (it != pendingMessages.end()) {
            std::string msg = it->second;

            Ptr<Packet> packet = Create<Packet>((uint8_t *)msg.c_str(), msg.size());
            socket->Send(packet);

            Simulator::Schedule(Seconds(30.0), &TcpGossipApp::CloseSocket, this, socket);
            pendingMessages.erase(it);
        }
    }

    void CloseSocket(Ptr<Socket> socket) {
        socket->Close();
        socketToAddress.erase(socket);
    }

    void SetSender() { m_isSender = true; }

    const std::unordered_set<std::string>& GetReceivedMessages() const {
        return receivedMessages;
    }

    void PrintReceivedMessages() const {
        std::cout << "Node " << m_nodeId << " received messages:\n";
        for (const auto& msg : receivedMessages) {
            std::cout << "  - " << msg << "\n";
        }
    }

private:
    uint32_t ExtractNodeIdFromIpv4(Ipv4Address addr) {
        std::stringstream ss;
        ss << addr;
        std::string ipStr = ss.str();

        size_t lastDot = ipStr.find_last_of('.');
        if (lastDot != std::string::npos && lastDot + 1 < ipStr.length()) {
            return std::stoi(ipStr.substr(lastDot + 1));
        }
        return 0;
    }
};

class MinerApp : public Application {
    private:
        EventId m_miningEvent;
        uint32_t m_blockCounter = 0;
        bool m_running = false;
        Ptr<TcpGossipApp> m_gossipApp;
        double m_stopMiningTime = 0.0;
    
    public:
        MinerApp() {}
    
        static uint32_t totalBlocksMined;
        static std::map<uint32_t, uint32_t> perNodeMinedBlocks;
    
        virtual void StartApplication() override {
            NS_LOG_INFO("MinerApp started on node " << GetNode()->GetId());
            m_running = true;
    
            // Stop mining 5 seconds before simulation ends
            // m_stopMiningTime = Simulator::GetStopTime().GetSeconds() - 5.0;
    
            ScheduleNextMining();
        }
    
        virtual void StopApplication() override {
            m_running = false;
            if (m_miningEvent.IsRunning()) {
                Simulator::Cancel(m_miningEvent);
            }
        }
    
        void SetSimulationStopTime(double stopTime) {
            m_stopMiningTime = stopTime - 20.0;
        }
        
        void SetGossipApp(Ptr<TcpGossipApp> app) {
            m_gossipApp = app;
        }
    
        uint32_t GetBlocksMined() const {
            return m_blockCounter;
        }
    
    private:
        void ScheduleNextMining() {
            if (!m_running) return;
    
            double interval = 10 + (rand() % 5); // 10–14s random delay
            double nextMiningTime = Simulator::Now().GetSeconds() + interval;
    
            if (nextMiningTime < m_stopMiningTime) {
                m_miningEvent = Simulator::Schedule(Seconds(interval), &MinerApp::MineBlock, this);
            } else {
                NS_LOG_INFO("Miner " << GetNode()->GetId() << " will not mine further to allow propagation.");
            }
        }
    
        void MineBlock() {
            if (!m_running) return;
            if (Simulator::Now().GetSeconds() >= m_stopMiningTime) {
                NS_LOG_INFO("Too late to mine, skipping");
                return;
            }

            m_blockCounter++;
            std::string blockMsg = "Block_" + std::to_string(m_blockCounter) + "_" + std::to_string(GetNode()->GetId());
    
            NS_LOG_INFO("Miner " << GetNode()->GetId() << " mined: " << blockMsg);
    
            totalBlocksMined++;
            perNodeMinedBlocks[GetNode()->GetId()]++;
    
            if (m_gossipApp) {
                m_gossipApp->SendMessage(blockMsg);
            }
    
            ScheduleNextMining();
        }
    };
    
    // Static member initialization
    uint32_t MinerApp::totalBlocksMined = 0;
    std::map<uint32_t, uint32_t> MinerApp::perNodeMinedBlocks;
    
    

int main(int argc, char *argv[]) {
    
    
    srand(time(NULL)); // ✅ Seed the RNG only once at the beginning
    
    
    uint32_t numNodes = 20;
    uint32_t no_of_peers = 8;
    double simulationTime = 60.0;

    LogComponentEnable("TcpGossip", LOG_LEVEL_INFO);

    NodeContainer nodes;
    nodes.Create(numNodes);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel");
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.0.0", "255.255.0.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    std::vector<Ptr<TcpGossipApp>> gossipApps(numNodes);
    std::vector<Ptr<MinerApp>> minerApps(numNodes);

    for (uint32_t i = 0; i < numNodes; i++) {
        gossipApps[i] = CreateObject<TcpGossipApp>(interfaces.GetAddress(i));
        nodes.Get(i)->AddApplication(gossipApps[i]);
        gossipApps[i]->SetStartTime(Seconds(0.5));
    }

    for (uint32_t i = 0; i < numNodes; i++) {
        std::unordered_set<uint32_t> selected;
        while (static_cast<int>(selected.size()) < no_of_peers) {
            uint32_t neighbor = rand() % numNodes;
            if (neighbor != i && selected.find(neighbor) == selected.end()) {
                gossipApps[i]->AddNeighbor(interfaces.GetAddress(neighbor));
                selected.insert(neighbor);
            }
        }
    }

    for (uint32_t i = 0; i < numNodes; ++i) {
        minerApps[i] = CreateObject<MinerApp>();
        minerApps[i]->SetGossipApp(gossipApps[i]);
        minerApps[i]->SetSimulationStopTime(simulationTime);
        nodes.Get(i)->AddApplication(minerApps[i]);
        minerApps[i]->SetStartTime(Seconds(1.0 + (i * 0.01)));
        
    }

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // Print summary of blocks mined
    std::cout << "\n=== BLOCKCHAIN NETWORK SUMMARY ===\n";
    std::cout << "Total blocks mined across network: " << MinerApp::totalBlocksMined << "\n\n";
    
    // Print per-node mining statistics
    std::cout << "Blocks mined by each node:\n";
    for (const auto& pair : MinerApp::perNodeMinedBlocks) {
        std::cout << "  Node " << pair.first << ": " << pair.second << " blocks\n";
    }
    
    // Alternative way to count total blocks (by counting unique blocks in the network)
    std::unordered_set<std::string> uniqueBlocks;
    for (auto& app : gossipApps) {
        const auto& messages = app->GetReceivedMessages();
        uniqueBlocks.insert(messages.begin(), messages.end());
    }
    
    std::cout << "\nUnique blocks propagated in network: " << uniqueBlocks.size() << "\n";
    
    // Print received messages for each node
    for (auto& app : gossipApps) {
        app->PrintReceivedMessages();
    }

    Simulator::Destroy();
    return 0;
}
