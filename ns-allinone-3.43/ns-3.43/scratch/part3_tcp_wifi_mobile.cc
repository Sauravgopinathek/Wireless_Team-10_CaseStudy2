/*
 * part3_tcp_wifi_mobile.cc
 * 23CSE343 Wireless & Mobile Networks - Case Study 2, Part 3 (Team 10)
 *
 * TCP over an 802.11n Wi-Fi link with a mobile station.
 *   Node 0 = Access Point (TCP receiver / PacketSink)
 *   Node 1 = Mobile STA   (TCP sender / BulkSend)
 *
 * Written for ns-3.36 or newer (uses WIFI_STANDARD_80211n).
 *
 * Design 1 (static STA):
 *   ./ns3 run "scratch/part3_tcp_wifi_mobile --speed=0 --tag=design1"
 * Design 2 (STA moves away, turns round at half time, comes back):
 *   ./ns3 run "scratch/part3_tcp_wifi_mobile --speed=4 --tag=design2"
 *
 * Output files (written to the directory you run from):
 *   <tag>-throughput.dat   time  distance  throughput(Mbps)   (every 1 s)
 *   <tag>-cwnd.dat         time  cwnd(bytes)                    (every change)
 *   <tag>-flowmon.xml      FlowMonitor statistics
 *   part3-<tag>-0-0.pcap   packet capture at the AP (open in Wireshark)
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

#include <fstream>
#include <iomanip>
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Part3TcpWifiMobile");

static Ptr<PacketSink> g_sink;
static Ptr<MobilityModel> g_apMob;
static Ptr<MobilityModel> g_staMob;
static uint64_t g_lastRx = 0;
static std::ofstream g_thrFile;
static std::ofstream g_cwndFile;

// Called every second: throughput received at the AP + current distance
static void
LogThroughput()
{
    uint64_t total = g_sink->GetTotalRx();
    double mbps = (total - g_lastRx) * 8.0 / 1e6; // bytes in the last 1 s -> Mbps
    g_lastRx = total;
    double dist = g_staMob->GetDistanceFrom(g_apMob);
    double t = Simulator::Now().GetSeconds();

    std::cout << std::fixed << std::setprecision(2) << t << "\t" << dist << "\t" << mbps
              << std::endl;
    g_thrFile << std::fixed << std::setprecision(3) << t << "\t" << dist << "\t" << mbps
              << std::endl;

    Simulator::Schedule(Seconds(1.0), &LogThroughput);
}

// Congestion-window trace of the sender's TCP socket
static void
CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
{
    g_cwndFile << std::fixed << std::setprecision(4) << Simulator::Now().GetSeconds() << "\t"
               << newCwnd << std::endl;
}

// The TCP socket only exists after BulkSend starts, so connect the trace afterwards
static void
ConnectCwndTrace()
{
    Config::ConnectWithoutContext("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                                  MakeCallback(&CwndChange));
}

int
main(int argc, char* argv[])
{
    std::string tcpVariant = "TcpNewReno";
    std::string tag = "design1";
    double simTime = 60.0;   // seconds
    double speed = 0.0;      // m/s (0 = static STA)
    double startDist = 5.0;  // initial distance STA <-> AP in metres
    uint32_t segSize = 1448; // TCP segment size in bytes
    bool pcap = true;

    CommandLine cmd(__FILE__);
    cmd.AddValue("tcp", "TCP variant: TcpNewReno, TcpWestwood, TcpCubic, TcpLinuxReno ...", tcpVariant);
    cmd.AddValue("tag", "Label used in output file names", tag);
    cmd.AddValue("simTime", "Simulation time in seconds", simTime);
    cmd.AddValue("speed", "STA speed in m/s (0 = static)", speed);
    cmd.AddValue("startDist", "Initial STA-AP distance in metres", startDist);
    cmd.AddValue("pcap", "Enable PCAP tracing at the AP", pcap);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::" + tcpVariant));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segSize));

    // ---- Nodes: 0 = AP, 1 = STA ----
    NodeContainer nodes;
    nodes.Create(2);
    Ptr<Node> apNode = nodes.Get(0);
    Ptr<Node> staNode = nodes.Get(1);

    // ---- Wi-Fi (802.11n, log-distance path loss) ----
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager("ns3::IdealWifiManager"); // rate adapts to SNR

    WifiMacHelper mac;
    Ssid ssid = Ssid("case-study-2");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDev = wifi.Install(phy, mac, staNode);
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDev = wifi.Install(phy, mac, apNode);

    // ---- Mobility: AP fixed at origin, STA moves along the x axis ----
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(apNode);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(staNode);

    Ptr<ConstantPositionMobilityModel> apMob = apNode->GetObject<ConstantPositionMobilityModel>();
    apMob->SetPosition(Vector(0.0, 0.0, 0.0));
    Ptr<ConstantVelocityMobilityModel> staMob = staNode->GetObject<ConstantVelocityMobilityModel>();
    staMob->SetPosition(Vector(startDist, 0.0, 0.0));
    staMob->SetVelocity(Vector(speed, 0.0, 0.0));
    if (speed > 0)
    {
        // turn round at half time and come back towards the AP
        Simulator::Schedule(Seconds(simTime / 2.0),
                            &ConstantVelocityMobilityModel::SetVelocity,
                            staMob,
                            Vector(-speed, 0.0, 0.0));
    }
    g_apMob = apMob;
    g_staMob = staMob;

    // ---- Internet stack + IP addresses ----
    InternetStackHelper stack;
    stack.Install(nodes);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apIf = address.Assign(apDev);
    address.Assign(staDev);

    // ---- TCP applications: STA sends (BulkSend), AP receives (PacketSink) ----
    uint16_t port = 9;
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkHelper.Install(apNode);
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simTime));
    g_sink = DynamicCast<PacketSink>(sinkApp.Get(0));

    BulkSendHelper bulk("ns3::TcpSocketFactory", InetSocketAddress(apIf.GetAddress(0), port));
    bulk.SetAttribute("MaxBytes", UintegerValue(0)); // send until the end
    ApplicationContainer srcApp = bulk.Install(staNode);
    srcApp.Start(Seconds(1.0));
    srcApp.Stop(Seconds(simTime));

    // ---- Tracing / statistics ----
    g_thrFile.open(tag + "-throughput.dat");
    g_cwndFile.open(tag + "-cwnd.dat");
    Simulator::Schedule(Seconds(1.01), &ConnectCwndTrace);
    Simulator::Schedule(Seconds(2.0), &LogThroughput); // first sample covers 1 s..2 s

    if (pcap)
    {
        phy.EnablePcap("part3-" + tag, apDev.Get(0));
    }

    FlowMonitorHelper fmHelper;
    Ptr<FlowMonitor> monitor = fmHelper.InstallAll();

    std::cout << "TCP variant: " << tcpVariant << " | tag: " << tag << " | simTime: " << simTime
              << " s | speed: " << speed << " m/s | start distance: " << startDist << " m"
              << std::endl;
    std::cout << "time(s)\tdist(m)\tthroughput(Mbps)" << std::endl;

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // ---- FlowMonitor summary ----
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(fmHelper.GetClassifier());
    std::cout << "\n=== FlowMonitor results ===" << std::endl;
    for (const auto& kv : monitor->GetFlowStats())
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(kv.first);
        const FlowMonitor::FlowStats& s = kv.second;
        double dur = s.timeLastRxPacket.GetSeconds() - s.timeFirstTxPacket.GetSeconds();
        double thr = (dur > 0) ? s.rxBytes * 8.0 / dur / 1e6 : 0.0;
        double delayMs = (s.rxPackets > 0) ? s.delaySum.GetSeconds() * 1000.0 / s.rxPackets : 0.0;
        double lossPct = (s.txPackets > 0) ? 100.0 * s.lostPackets / s.txPackets : 0.0;
        std::cout << "Flow " << kv.first << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << ")" << std::endl;
        std::cout << "  Tx packets: " << s.txPackets << "  Rx packets: " << s.rxPackets
                  << "  Lost: " << s.lostPackets << "  Loss: " << std::fixed
                  << std::setprecision(2) << lossPct << " %" << std::endl;
        std::cout << "  Mean delay: " << delayMs << " ms  Throughput: " << thr << " Mbps"
                  << std::endl;
    }
    std::cout << "Total bytes received by sink: " << g_sink->GetTotalRx() << std::endl;
    std::cout << "Average application throughput: "
              << g_sink->GetTotalRx() * 8.0 / (simTime - 1.0) / 1e6 << " Mbps" << std::endl;

    monitor->SerializeToXmlFile(tag + "-flowmon.xml", true, true);

    g_thrFile.close();
    g_cwndFile.close();
    Simulator::Destroy();
    return 0;
}
