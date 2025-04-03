/**
 *  A simple implementation of gossip protocol where node 0 is main sender of messages that propagate throughout the network.  
 *  You can think of it as node 0 mined a share and now it is propagated throughout the network.
 * 
 * 
 *  This code is a simulation of a gossip protocol using TCP sockets in ns-3.
 *  it uses Wireless ad hoc networks to simulate a peer-to-peer network.
 */

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
     std::unordered_set<std::string> receivedMessages;
     std::unordered_set<std::string> forwardedMessages;
     std::unordered_set<Ptr<Socket>> m_connectedSockets;
     Ipv4Address m_myAddress;
     uint32_t m_nodeId;
     bool m_isSender;
 
 public:
     
     // Constructor - Initializes the app with the node's IP address    
     TcpGossipApp(Ipv4Address myAddress) : m_myAddress(myAddress), m_isSender(false) {}
 
     // Adds a neighbor to this node's peer list
     void AddNeighbor(Ipv4Address neighbor) {
         if (neighbor != m_myAddress) {  // Don't add self as neighbor
             m_neighbors.push_back(neighbor);
         }
     }
 
 
     // Called when the application starts
     // Sets up the listening socket and callbacks
     void StartApplication() override {
         m_nodeId = GetNode()->GetId();  // Store the node ID for logging
 
         // Create and configure the listening socket
         m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
         m_socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 8080));  // Bind to any IP, port 8080
         m_socket->Listen();  // Start listening for connections
         
         // Set up callbacks for accepting connections and receiving data
         m_socket->SetAcceptCallback(
             MakeCallback(&TcpGossipApp::AcceptConnection, this),
             MakeCallback(&TcpGossipApp::HandleAccept, this)
         );
         m_socket->SetRecvCallback(MakeCallback(&TcpGossipApp::ReceiveMessage, this));
 
         // If this node is designated as the initial sender, schedule the first message (node 0 only for now)
         if (m_isSender) {
             Simulator::Schedule(Seconds(1.0), &TcpGossipApp::SendMessage, this, "Block 1 mined");
         }
     }
 
     // THESE 2 are for the receive function
     // Callback for deciding whether to accept an incoming connection (always true for now)
     bool AcceptConnection(Ptr<Socket> socket, const Address &from) {
         return true;  // Always accept connections from any node
     }
 
     
     // Callback for handling a newly accepted connection
     void HandleAccept(Ptr<Socket> socket, const Address &from) {
         m_connectedSockets.insert(socket);  // Add to set of connected sockets
         socket->SetRecvCallback(MakeCallback(&TcpGossipApp::ReceiveMessage, this));  // Set up receive callback
     }
 
     
     // Sends a message to all neighbors
     void SendMessage(std::string msg) {
         // Avoid processing duplicate messages
         if (receivedMessages.count(msg) > 0) return;
         
         // Mark message as received and forwarded
         receivedMessages.insert(msg);
         forwardedMessages.insert(msg);  // Mark as forwarded immediately to prevent duplicates
 
         NS_LOG_INFO("Node " << m_nodeId << " sending message: " << msg);
         
         // Create a new socket for each neighbor and initiate connection
         for (const auto &neighbor : m_neighbors) {
             Ptr<Socket> sendSocket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
             sendSocket->SetConnectCallback(
                 MakeCallback(&TcpGossipApp::HandleConnected, this),  // Called when connection is established
                 MakeNullCallback<void, Ptr<Socket>>()                // No callback for connection failure for now this was working so I didn't want to change it
             );
             sendSocket->Connect(InetSocketAddress(neighbor, 8080));  // Connect to neighbor
             pendingMessages[sendSocket] = msg;  // Store message to send when connected
             socketToAddress[sendSocket] = neighbor;  // Track which address this socket connects to
         }
     }
 
     
     // Callback for when a connection is successfully established
     void HandleConnected(Ptr<Socket> socket) {
         auto it = pendingMessages.find(socket);
         if (it != pendingMessages.end()) {
             std::string msg = it->second;
             
             // Creates and sends the packet with the message
             Ptr<Packet> packet = Create<Packet>((uint8_t *)msg.c_str(), msg.size());
             socket->Send(packet);
         
             // Schedule socket closure after 30 seconds to clean up resources
             Simulator::Schedule(Seconds(30.0), &TcpGossipApp::CloseSocket, this, socket);
             pendingMessages.erase(it);  // Remove from pending messages
         }
     }
     
     
     // Closes a socket and cleans up tracking data
     void CloseSocket(Ptr<Socket> socket) {
         socket->Close();
         socketToAddress.erase(socket);  // Clean up our tracking map
     }
 
     
     // Handling receiving messages
     void ReceiveMessage(Ptr<Socket> socket) {
         // Getting the sender's address
         Address from;
         socket->GetPeerName(from);
         Ipv4Address senderAddress = InetSocketAddress::ConvertFrom(from).GetIpv4();
         
         // Receive the packet
         Ptr<Packet> packet = socket->Recv();
         if (!packet || packet->GetSize() == 0) return;  // Skip empty packets
     
         // Extract message content from packet
         uint32_t size = packet->GetSize();
         std::vector<uint8_t> buffer(size);
         packet->CopyData(buffer.data(), size);
         std::string msg(buffer.begin(), buffer.end());
     
         // Extract a node ID from the sender IP for better logging
         uint32_t senderNodeId = ExtractNodeIdFromIpv4(senderAddress);
         NS_LOG_INFO("Node " << m_nodeId << " received message from Node " << senderNodeId);
         
         // Only forward if we haven't seen AND haven't forwarded this message before
         if (receivedMessages.count(msg) == 0 && forwardedMessages.count(msg) == 0) { 
             receivedMessages.insert(msg);
             // Schedule forwarding with a small random delay (10-30ms) to prevent network congestion
             Simulator::Schedule(MilliSeconds(10 + rand() % 20), &TcpGossipApp::ForwardMessage, this, msg);
         }
     }
 
 
     // Forwards a message to all neighbors
     void ForwardMessage(std::string msg) {
         // Avoid forwarding the same message multiple times 
         if (forwardedMessages.count(msg) > 0) {
             return;
         }
         
         forwardedMessages.insert(msg);
     
         // NS_LOG_INFO("Node " << m_nodeId << " forwarding message: " << msg); // Log the forwarding action
     
         // Create a new socket for each neighbor and forward the message
         for (const auto &neighbor : m_neighbors) {
             Ptr<Socket> sendSocket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
             sendSocket->SetConnectCallback(
                 MakeCallback(&TcpGossipApp::HandleConnected, this),
                 MakeNullCallback<void, Ptr<Socket>>()
             );
         
             sendSocket->Connect(InetSocketAddress(neighbor, 8080));
             pendingMessages[sendSocket] = msg;
             socketToAddress[sendSocket] = neighbor;  // Store which address this socket connects to
         }
     }
     
     // Used by node 0 to assign itself as sender     
     void SetSender() { m_isSender = true; }
 
 private:
     // Helper functions 
     uint32_t ExtractNodeIdFromIpv4(Ipv4Address addr) {
         std::stringstream ss;
         ss << addr;
         std::string ipStr = ss.str();
         
         // Simple approach: extract the last number from the IP
         size_t lastDot = ipStr.find_last_of('.');
         if (lastDot != std::string::npos && lastDot + 1 < ipStr.length()) {
             std::string nodeIdStr = ipStr.substr(lastDot + 1);
             return std::stoi(nodeIdStr);
         }
         
         return 0;  // Default if parsing fails
     }
 };
 
 
 int main(int argc, char *argv[]) {
     // Simulation parameters
     uint32_t numNodes = 500;  // Total number of nodes in the network 
     uint32_t no_of_peers = 8; // Number of peers each node connects to
                               // (to add randomness, use: "+ rand() % x" where x = desired range)
     double simulationTime = 60.0;  // Duration of simulation in seconds
 
     // Enable logging for this component
     LogComponentEnable("TcpGossip", LOG_LEVEL_INFO);
 
     // Create nodes
     NodeContainer nodes;
     nodes.Create(numNodes);
 
     // Set up WiFi network
     WifiHelper wifi;
     wifi.SetStandard(WIFI_STANDARD_80211b);  // Use 802.11b standard
     
     // Configure the physical layer
     YansWifiPhyHelper wifiPhy;
     YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
     wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");  // Constant propagation delay
     wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel");      // Signal attenuation with distance
     wifiPhy.SetChannel(wifiChannel.Create());
     
     // Configure the MAC layer
     WifiMacHelper wifiMac;
     wifiMac.SetType("ns3::AdhocWifiMac");  // Use ad-hoc mode (no access point)
     
     // Install WiFi on all nodes
     NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);
 
     // Set up internet stack
     InternetStackHelper internet;
     internet.Install(nodes);
     
     // Assign IP addresses
     Ipv4AddressHelper ipv4;
     ipv4.SetBase("10.1.0.0", "255.255.0.0");  // Uses 10.1.x.x address range
     Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);
 
     // Set up node positions (stationary nodes)
     MobilityHelper mobility;
     mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
     mobility.Install(nodes);
 
     // Create gossip applications for each node
     std::vector<Ptr<TcpGossipApp>> gossipApps(numNodes);
     for (uint32_t i = 0; i < numNodes; i++) {
         gossipApps[i] = CreateObject<TcpGossipApp>(interfaces.GetAddress(i));
         nodes.Get(i)->AddApplication(gossipApps[i]);
     }
 
     // For each node, randomly select peers to connect with
     for (uint32_t i = 0; i < numNodes; i++) {
         std::unordered_set<uint32_t> selected;
         while (static_cast<int>(selected.size()) < no_of_peers) {
             uint32_t neighbor = rand() % numNodes;  // Randomly select a neighbor
             if (neighbor != i) {  // Don't connect to self
                 gossipApps[i]->AddNeighbor(interfaces.GetAddress(neighbor));
                 selected.insert(neighbor);
             }
         }
     }
 
     // Designate node 0 as the initial message sender
     gossipApps[0]->SetSender();
     
     // Start all applications at 0.5 seconds into the simulation
     for (uint32_t i = 0; i < numNodes; i++) {
         gossipApps[i]->SetStartTime(Seconds(0.5));
     }
 
     // Run the simulation
     Simulator::Stop(Seconds(simulationTime));
     Simulator::Run();
     Simulator::Destroy();
     return 0;
 }