// ./waf --run "scratch/mobTop24 --nodeNum=24 --traceFile=scratch/sc-24nodes.tcl" > log.out 2>&1


#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/social-network.h"

//XXX For anumation
#include "ns3/netanim-module.h"

using namespace std;

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Nhap9");



static void
CourseChange (std::ostream *myos, std::string foo, Ptr<const MobilityModel> mobility)
{
  Ptr<Node> node = mobility->GetObject<Node> ();
  Vector pos = mobility->GetPosition (); // Get position
  Vector vel = mobility->GetVelocity (); // Get velocity
  
  std::cout.precision(5);
  *myos << Simulator::Now () << "; NODE: " << node->GetId() << "; POS: x=" << pos.x << ", y=" << pos.y
	<< ", z=" << pos.z << "; VEL: x=" << vel.x << ", y=" << vel.y
	<< ", z=" << vel.z << std::endl;
}

class Topology
{
public:
    Topology ();
    bool Configure (int argc, char *argv[]);
    void Run ();
    std::string logFile;
    std::ofstream myos;
    std::string traceFile;
    void PrintNames ();

private:
    uint32_t nodeNum;
	uint32_t requestors;
    double duration;
    bool pcap;
    bool verbose;

    NodeContainer nodes;
    NetDeviceContainer devices;
    Ipv4InterfaceContainer interfaces;

private:
    void CreateNodes ();
    void CreateDevices ();
    void InstallInternetStack ();
    void InstallApplications ();
	Ipv4Address GetIpv4Address(uint32_t index);
};

Topology::Topology (): nodeNum (3), requestors (5), duration (60.0),
	pcap (false),verbose (true)
{
}

bool Topology::Configure (int argc, char *argv[])
{
    CommandLine cmd;

    cmd.AddValue ("nodeNum", "Number of nodes.", nodeNum);
    cmd.AddValue ("duration", "Simulation time in sec.", duration);
    cmd.AddValue ("traceFile", "NS3 mobility trace.", traceFile);
    cmd.AddValue ("logFile", "Log file", logFile);
    cmd.AddValue ("verbose", "Tell application to log if true", verbose);
	cmd.AddValue ("requestors", "Number of requesting nodes.", requestors);


    if (verbose)
    {
        LogComponentEnable ("SocialNetworkApplication", LOG_LEVEL_INFO);
    }

    cmd.Parse (argc, argv);
    return true;
}

void Topology::Run ()
{
    CreateNodes ();
    CreateDevices ();
    InstallInternetStack ();
    InstallApplications ();

    std::cout << "Starting simulation for " << duration << " s.\n";

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    Simulator::Stop (Seconds (duration));


    for (uint32_t i=0; i < nodeNum; i++)
    {
        //if (i < nodeNum/2)
        if (i < 50)
            AnimationInterface::SetNodeColor (nodes.Get(i), 0, 255, 0);
        else
            AnimationInterface::SetNodeColor (nodes.Get(i), 0, 0, 255);
	}

//	AnimationInterface::SetNodeColor (nodes.Get(3), 255, 0, 55);
//	AnimationInterface::SetNodeColor (nodes.Get(38), 255, 0, 55);
//	AnimationInterface::SetNodeColor (nodes.Get(45), 100, 150, 155);
//	AnimationInterface::SetNodeColor (nodes.Get(11), 10, 10, 155);

    AnimationInterface anim ("mobTop5.xml");
    //anim.EnablePacketMetadata (true);
    Simulator::Run ();
    myos.close (); // close log file
    Simulator::Destroy ();
}

// XXX: I am thinking that we can create a sepreate set of nodes for each community
//      and set up a different mobility trace for each community
void Topology::CreateNodes ()
{
    Ns2MobilityHelper mob = Ns2MobilityHelper (traceFile);
    myos.open (logFile.c_str ());
    std::cout << "Creating " << nodeNum << " nodes.\n";
    nodes.Create (nodeNum);

    // Name nodes
    /*for (uint32_t i = 0; i < nodeNum/2; i++)
    {
        std::ostringstream os1, os2;
        os1 << "Comm1-" << i;
        Names::Add (os1.str (), nodes.Get (i));
        os2 << "Comm2-" << i;
        Names::Add (os2.str (), nodes.Get (nodeNum/2 + i));
    }*/
    

    mob.Install ();
    Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
		   MakeBoundCallback (&CourseChange, &myos));
}

void Topology::CreateDevices ()
{
    std::string phyMode ("DsssRate1Mbps");

    // disable fragmentation for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
    // turn off RTS/CTS for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));

    
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    channel.AddPropagationLoss("ns3::RangePropagationLossModel",
                               "MaxRange", DoubleValue(10.0)); //XXX
   

    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (channel.Create ());

    WifiHelper wifi = WifiHelper::Default ();
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                  "DataMode",StringValue (phyMode),
                                  "ControlMode",StringValue (phyMode));
    
    NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

    mac.SetType ("ns3::AdhocWifiMac");

    devices = wifi.Install (phy, mac, nodes);

    if (pcap)
    {
        phy.EnablePcapAll (std::string ("SocialTie"));
    }

    /*/ Take this out _____________________________________//
     MobilityHelper mobility;

    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (5.0),
                                   "DeltaY", DoubleValue (10.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (nodes);

    //_____________________________________________________*/
}

void Topology::InstallInternetStack ()
{
    InternetStackHelper stack;
    stack.Install (nodes);
    Ipv4AddressHelper address;
    address.SetBase ("1.0.0.0", "255.0.0.0");
    interfaces = address.Assign (devices);
}

void Topology::InstallApplications ()
{
    Ptr<SocialNetwork> app[nodeNum];
    for (uint32_t i =0; i < nodeNum; i++)
    {   
        app[i] = CreateObject<SocialNetwork> ();
        app[i]->Setup (9);
        nodes.Get (i)->AddApplication (app[i]);
        app[i]->SetStartTime (Seconds (0.5 + 0.0001*i));
        app[i]->SetStopTime (Seconds (duration-0.5));
	if (i<50) {
		app[i]->m_communityId = 0;
	}
	else {
		app[i]->m_communityId = 1;
	}
    }



	// Use one node or multiple nodes VVV

	if(true) // Change to true for single node
	{
		app[55]->RequestContent(GetIpv4Address(79));
		app[27]->RequestContent(GetIpv4Address(9));
		app[65]->RequestContent(GetIpv4Address(5));  //inter
		app[45]->RequestContent(GetIpv4Address(7));
		app[20]->RequestContent(GetIpv4Address(55)); //inter
		app[30]->RequestContent(GetIpv4Address(60)); //inter
		app[18]->RequestContent(GetIpv4Address(6));
		app[12]->RequestContent(GetIpv4Address(19));
		app[16]->RequestContent(GetIpv4Address(70)); //inter
		app[15]->RequestContent(GetIpv4Address(17));
		app[33]->RequestContent(GetIpv4Address(100)); //inter
		app[80]->RequestContent(GetIpv4Address(21)); //inter
		app[56]->RequestContent(GetIpv4Address(40)); //inter
		app[101]->RequestContent(GetIpv4Address(98));
		app[110]->RequestContent(GetIpv4Address(43));//inter
		app[90]->RequestContent(GetIpv4Address(39)); //inter
		app[34]->RequestContent(GetIpv4Address(72)); //inter
		app[29]->RequestContent(GetIpv4Address(87)); //inter
		app[69]->RequestContent(GetIpv4Address(55));
		app[70]->RequestContent(GetIpv4Address(19)); //inter
		app[7]->RequestContent(GetIpv4Address(101)); //inter
		app[50]->RequestContent(GetIpv4Address(78));
		app[89]->RequestContent(GetIpv4Address(115));
		app[47]->RequestContent(GetIpv4Address(66)); //inter
		app[83]->RequestContent(GetIpv4Address(62));
		app[97]->RequestContent(GetIpv4Address(17)); //inter
		app[105]->RequestContent(GetIpv4Address(1)); //inter
		app[4]->RequestContent(GetIpv4Address(60));  //inter
		app[42]->RequestContent(GetIpv4Address(46));
		app[63]->RequestContent(GetIpv4Address(3));  //inter

	}
	else
	{
		srand (time(NULL));

		// Number of requesting applications
		uint32_t i = 0;
		while (i < requestors)
		{
			uint32_t requNode = rand () % nodeNum;
			uint32_t contNode = rand () % nodeNum;
			if (requNode != contNode)
			{
				cout << "Requestor\t" << requNode << "\tIP\t" << GetIpv4Address(requNode) 
					<< "\tContent\t" << GetIpv4Address(contNode) << endl;
				app[requNode]->RequestContent(GetIpv4Address(contNode));
				i++;
			}
		}
	}
}

void Topology::PrintNames ()
{
    for (uint32_t i=0; i< nodeNum; i++)
        std::cout << Names::FindName (nodes.Get(i)) << std::endl;
}

Ipv4Address Topology::GetIpv4Address(uint32_t index)
{
	return nodes.Get(index)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
}

int main (int argc, char *argv[])
{
    Topology test;
    if (! test.Configure (argc, argv))
        NS_FATAL_ERROR ("Configuration failed. Aborted.");

    test.Run ();

    return 0;
}
