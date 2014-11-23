
// TUAN: WRITE TO LOG FILE
//./waf --run scratch/topology6 >log.out 2>&1
//./waf --run "scratch/mobTop5 --nodeNum=50 --traceFile=scratch/sc2.tcl" > log.out 2>&1
//./waf --run "scratch/tuan_topology0 --nodeNum=2 --traceFile=scratch/sc2.tcl" > log.out 2>&1
//./waf --run "scratch/mobTop7-15 --nodeNum=120 --traceFile=scratch/sc10.tcl" > log.out 2>&1
//./waf --run "scratch/mobTop170 --nodeNum=170 --traceFile=scratch/sc170.tcl" > log.out 2>&1




/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/social-network.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/address-utils.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"

//TUAN
#include "ns3/string.h"
#include "ns3/pkt-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4.h"
//TUAN

using namespace std;

namespace ns3 {

uint32_t SocialNetwork::global_count_hello = 0;
uint32_t SocialNetwork::global_count_interest = 0;
uint32_t SocialNetwork::global_count_data = 0;

NS_LOG_COMPONENT_DEFINE ("SocialNetworkApplication");
NS_OBJECT_ENSURE_REGISTERED (SocialNetwork);

TypeId
SocialNetwork::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3:SocialNetwork")
    .SetParent<Application> ()
    .AddConstructor<SocialNetwork> ()
    .AddAttribute ("MaxPackets", 
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&SocialNetwork::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval", 
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&SocialNetwork::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemotePort", 
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SocialNetwork::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&SocialNetwork::SetDataSize,
                                         &SocialNetwork::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("RequestedContent",
                   "The initial requested content",
                   Ipv4AddressValue ("0.0.0.0"),
                   MakeIpv4AddressAccessor (&SocialNetwork::m_initialRequestedContent),
                   MakeIpv4AddressChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&SocialNetwork::m_txTrace))
  ;
  return tid;
}

SocialNetwork::SocialNetwork ()
{
    NS_LOG_FUNCTION (this);
    m_sent = 0;
    m_socket = 0;
    m_sendEvent = EventId ();
    m_data = 0;
    m_dataSize = 0;
    m_contentManager = new ContentManager();
    m_interestManager = new InterestManager();
    m_seenDataManager = new InterestManager();
    m_foreignCommunityManager = new CommunityManager();
    m_interestBroadcastId = 0;
    m_pendingDataResponse = new vector<PendingResponseEntry>;
    m_pending_content_dest_node_response = new vector<PendingResponseEntry>;
    m_pending_interest_response = new vector<PendingResponseEntry>;
    m_last_foreign_encounter_node = new Ipv4Address[100]; //99 communities. Index 0 is reserved.
    m_firstSuccess = false;
    m_meetForeign = false;
    for (uint32_t i=0; i<100; ++i)
    {
        m_last_foreign_encounter_node[i] = Ipv4Address("0.0.0.0");
    }
}

SocialNetwork::~SocialNetwork()
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;

    delete [] m_data;
    delete m_contentManager;
    delete m_interestManager;
    delete m_seenDataManager;
    delete m_foreignCommunityManager;
    delete m_relationship;
    delete m_pendingDataResponse;
    delete m_pending_content_dest_node_response;
    delete m_pending_interest_response;
    
    m_data = 0;
    m_dataSize = 0;
}

void
SocialNetwork::DoDispose (void)
{
    NS_LOG_FUNCTION (this);
    Application::DoDispose ();
}

void
SocialNetwork::Setup (uint16_t port)
{
    m_peerPort = port;
}

void
SocialNetwork::RequestContent (Ipv4Address content)
{
    m_initialRequestedContent = content;
}

void 
SocialNetwork::StartApplication (void)
{
    NS_LOG_FUNCTION (this);
    Ipv4Address broadcastAddr = Ipv4Address("255.255.255.255");
    Ipv4Address thisNodeAddress = GetNodeAddress();
    
    //We use subnet mask: 255.0.0.0
    //community 1: 1.x.x.x
    //community 2: 2.x.x.x
    //We use the upper 8 bits for community id.
    //m_communityId = thisNodeAddress.Get() >> 24;
    //NS_LOG_INFO(""<<thisNodeAddress<<" community ID: "<<m_communityId);
    
    //cannot obtain Ipv4Address in the constructor before application starts.
    NS_LOG_INFO("m_communityId = " << m_communityId);
    //Attempt to do so resulting in a runtime error.
    Peer thisPeer = {thisNodeAddress, m_communityId};
    m_relationship = new Relationship(thisPeer);
    
    //insert the initial content into my content list
    m_initialOwnedContent = thisNodeAddress;
    NS_LOG_INFO(""<<thisNodeAddress<<" content: "<<m_initialOwnedContent);
    ContentOwnerEntry content(thisNodeAddress, m_initialOwnedContent, m_communityId);
    m_contentManager->Insert(content);
    
    //insert the initial requested content into m_pending_interest_response
    if ( !( m_initialRequestedContent.IsEqual(Ipv4Address("0.0.0.0"))) )
    {
        NS_LOG_INFO(""<<thisNodeAddress<<" requests content: "<<m_initialRequestedContent);
        PendingResponseEntry entry(thisNodeAddress, Ipv4Address("255.255.255.255"),
                m_initialRequestedContent, m_interestBroadcastId, Ipv4Address("0.0.0.0"), m_communityId, Ipv4Address("0.0.0.0")); 
        m_pending_interest_response->push_back(entry);
        m_interestBroadcastId++;
        //Thus, when this node encounters a node with higher social level, it will send
        //the interest packet to that node.
    }
  
    if (m_socket == 0)
    {
        TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket (GetNode (), tid);
        m_socket->SetAllowBroadcast(true);
        InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_peerPort);
        m_socket->Bind (local);
        m_socket->Connect (InetSocketAddress (broadcastAddr, m_peerPort)); //Enable broadcast functionality for the server
    }

    m_socket->SetRecvCallback (MakeCallback (&SocialNetwork::HandleRead, this));
    
    NS_LOG_INFO ("Application starts at: " << Simulator::Now ().GetSeconds ());
    
    ScheduleTransmitHelloPackets(10000);
}

/*
 Schedule to transmit hello packets every TIME_INTERVAL=100ms,
 starting at the initial value of time_elapsed.
*/
void
SocialNetwork::ScheduleTransmitHelloPackets(int numberOfHelloEvents)
{
    uint16_t event_counter = 0;
    double time_elapsed = Simulator::Now ().GetSeconds () + 
            static_cast <double> (rand()) / static_cast <double> (RAND_MAX);
    /*
    There are 2 components for time_elapsed:
    - Simulator::Now ().GetSeconds () => so that a new node joins will not send a packet at time already in the past
               For example: If it joins at time 2 second, it does not make sense to transmit at time 0 second.
    - static_cast <double> (RAND_MAX) => ensure that 2 nodes join at the same time, will not transmit at the same time
                                         which results in packet collision => No node receives the HELLO packet.
    */
    const double TIME_INTERVAL = 0.3; //500ms
    
    while (event_counter < numberOfHelloEvents)
    {
        time_elapsed += TIME_INTERVAL;
        ScheduleTransmit(Seconds(time_elapsed), HELLO);
        event_counter++;
    }
}

void 
SocialNetwork::StopApplication ()
{

    NS_LOG_FUNCTION (this);

    if (m_socket != 0) 
    {
        m_socket->Close ();
        m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
        m_socket = 0;
    }

    Simulator::Cancel (m_sendEvent);
}

void 
SocialNetwork::SetDataSize (uint32_t dataSize)
{
    NS_LOG_FUNCTION (this << dataSize);

    //
    // If the client is setting the echo packet data size this way, we infer
    // that she doesn't care about the contents of the packet at all, so 
    // neither will we.
    //
    delete [] m_data;
    m_data = 0;
    m_dataSize = 0;
    m_size = dataSize;
}

uint32_t 
SocialNetwork::GetDataSize (void) const
{
    NS_LOG_FUNCTION (this);
    return m_size;
}

void 
SocialNetwork::SetFill (std::string fill)
{
    NS_LOG_FUNCTION (this << fill);

    uint32_t dataSize = fill.size () + 1;

    if (dataSize != m_dataSize)
    {
        delete [] m_data;
        m_data = new uint8_t [dataSize];
        m_dataSize = dataSize;
    }

    memcpy (m_data, fill.c_str (), dataSize);

    //
    // Overwrite packet size attribute.
    //
    m_size = dataSize;
}

void 
SocialNetwork::SetFill (uint8_t fill, uint32_t dataSize)
{
    NS_LOG_FUNCTION (this << fill << dataSize);
    if (dataSize != m_dataSize)
    {
        delete [] m_data;
        m_data = new uint8_t [dataSize];
        m_dataSize = dataSize;
    }

    memset (m_data, fill, dataSize);

    //
    // Overwrite packet size attribute.
    //
    m_size = dataSize;
}

void 
SocialNetwork::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
    NS_LOG_FUNCTION (this << fill << fillSize << dataSize);
    if (dataSize != m_dataSize)
    {
        delete [] m_data;
        m_data = new uint8_t [dataSize];
        m_dataSize = dataSize;
    }

    if (fillSize >= dataSize)
    {
        memcpy (m_data, fill, dataSize);
        m_size = dataSize;
        return;
    }

    //
    // Do all but the final fill.
    //
    uint32_t filled = 0;
    while (filled + fillSize < dataSize)
    {
        memcpy (&m_data[filled], fill, fillSize);
        filled += fillSize;
    }

    //
    // Last fill may be partial
    //
    memcpy (&m_data[filled], fill, dataSize - filled);

    //
    // Overwrite packet size attribute.
    //
    m_size = dataSize;
}

void 
SocialNetwork::ScheduleTransmit (Time dt, PacketType packetType)
{
    NS_LOG_FUNCTION (this << dt);
    m_sendEvent = Simulator::Schedule (dt, &SocialNetwork::Send, this, packetType);
}

void 
SocialNetwork::Send (PacketType packetType)
{
    NS_LOG_FUNCTION (this);

    //NS_ASSERT (m_sendEvent.IsExpired ()); //Take this out => this causes runtime error when we have
                                            //multiple events scheduled. Ex: schedule more than 1 HELLO packet

    Ptr<Packet> p;
    PktHeader *header;
    
    p = Create<Packet> (m_size);

    //Create packet header
    if (packetType == HELLO)
    {
        header = CreateHelloPacketHeader();
    }
    else
    {
    }
    
    SendPacket(*header);

    NS_LOG_INFO("");
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s node "<< GetNodeAddress() <<
                 " broadcasts " << GetPacketTypeName(packetType) << " on port " << m_peerPort);
}

//TUAN: I decide to have different create packet header functions since they require different
//input parameters.

//TUANTUAN: contentRequested must by dynamically allocated
//We pass by pointer here to maintain a dynamic allocated string value
//pass by value, string will be destroyed when the function exits, gets a runtime error!!
PktHeader *
SocialNetwork::CreateDataPacketHeader(Ipv4Address destinationId, Ipv4Address requesterId,
        Ipv4Address contentRequested, uint32_t requesterCommunityId, Ipv4Address foreignDestinationId,
        vector<Ipv4Address> &bestBorderNode, uint32_t broadcastId)
{
    PktHeader *header = new PktHeader();
    header->SetSource(GetNodeAddress());
    header->SetDestination(destinationId);
    header->SetRequesterId(requesterId);
    header->SetRequestedContent(contentRequested); //actually, it should send the content data
    header->SetCommunityId(m_communityId);
    header->SetRequesterCommunityId(requesterCommunityId);
    header->SetForeignDestinationId(foreignDestinationId);
    header->SetBestBorderNodeId(bestBorderNode);
        //we don't really need to include the contentRequested value in DATA packet header.
    header->SetPacketType(DATA);
    header->SetInterestBroadcastId(broadcastId);
    
    return header;
}

PktHeader *
SocialNetwork::CreateInterestPacketHeader(Ipv4Address requesterId, Ipv4Address destinationId,
                Ipv4Address contentProviderId, Ipv4Address contentRequested, uint32_t broadcastId,
                uint32_t requesterCommunityId, Ipv4Address foreignDestinationId, vector<Ipv4Address> &bestBorderNodeId)
{
    PktHeader *header = new PktHeader();
    header->SetSource(GetNodeAddress());
    header->SetDestination(destinationId);
    header->SetContentProviderId(contentProviderId);
    header->SetRequestedContent(contentRequested);
    
    //The node that has the content, when receiving this packet, it will check
    //the requesterId and broadcastId of the packet, if it already exists
    //in its interestManager, it will discard the packet even though it has DATA matches
    //that interest.
    header->SetRequesterId(requesterId); 
    header->SetInterestBroadcastId(broadcastId);
    header->SetRequesterCommunityId(requesterCommunityId);
    header->SetForeignDestinationId(foreignDestinationId);
    header->SetBestBorderNodeId(bestBorderNodeId);
    
    header->SetPacketType(INTEREST);
    
    return header;
}

PktHeader *
SocialNetwork::CreateHelloPacketHeader()
{
    PktHeader *header = new PktHeader();
    
    //Set address of social tie table in packet header
    //which allows its encounter to merge this table.
    header->SetSocialTieTable( (uint32_t *)(m_relationship->GetSocialTableAddress()) );
    header->SetSocialTieTableSize(m_relationship->GetSocialTableSize());
            
    //Set packet type in packet header
    header->SetPacketType(HELLO);
    
    //Set destination in packet header
    header->SetDestination(Ipv4Address("255.255.255.255"));

    //Set source in packet header
    header->SetSource(GetNodeAddress());

	//Set my community ID in packet header
	header->SetCommunityId(m_communityId);
    
    return header;
}

PktHeader *
SocialNetwork::CreateDigestPacketHeader(Ipv4Address destinationId)
{
    PktHeader *header = new PktHeader();
            
    //Allow other neighbor nodes to read the content array from this node
    header->SetContentArraySize(m_contentManager->GetContentArraySize());
    header->SetContentArray( (uint32_t *) (m_contentManager->GetContentArray()) );
    
    //Set packet type in packet header
    header->SetPacketType(DIGEST);
    
    //Set destination in packet header
    header->SetDestination(destinationId); //we unicast this Digest packet to destinationId

    //Set source in packet header
    header->SetSource(GetNodeAddress());
    
    return header;
}

Ipv4Address
SocialNetwork::GetNodeAddress()
{
    //TUAN -- Figure out the IP address of this current node
    Ptr<Node> node = GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    Ipv4Address thisIpv4Address = ipv4->GetAddress(1,0).GetLocal(); //the first argument is the interface index
                                       //index = 0 returns the loopback address 127.0.0.1
    return thisIpv4Address;
}

void
SocialNetwork::HandleRead (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    Ptr<Packet> packet;
    Address from;

    NS_LOG_INFO("");
    Ipv4Address thisIpv4Address = GetNodeAddress();

    while ((packet = socket->RecvFrom (from)))
    {
        //TUAN -- Read the destination address from the received packet
        PktHeader *header = new PktHeader();
        packet->PeekHeader(*header); //should not remove header because intermediate node will relay this packet.
        //Ipv4Address targetDest = header->GetDestination();
        //NS_LOG_INFO("TUAN: Destination in received packet header: "<<targetDest);
        
        //Read the packet type
        PacketType packetType = header->GetPacketType();
        
        NS_LOG_INFO ("This node: " << thisIpv4Address );
        NS_LOG_INFO ("Encounter node: "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () );
        NS_LOG_INFO ("Encounter time: " << Simulator::Now ().GetSeconds () );
        
        switch (packetType)
        {
            case HELLO:
                NS_LOG_INFO ("Receive message type: HELLO");
                HandleHello(header);
                break;
                
            case INTEREST:
                NS_LOG_INFO ("Receive message type: INTEREST");
                HandleInterest(header);
                break;
            
            case DATA:
                NS_LOG_INFO ("Receive message type: DATA");
                HandleData(header);
                break;
                
            case DIGEST:
                NS_LOG_INFO ("Receive message type: DIGEST");
                HandleDigest(header);
                break;
                
            default:
                return;
        }

   }
}

void
SocialNetwork::HandleData(PktHeader *header)
{
    NS_LOG_INFO("");
    NS_LOG_INFO("Inside HandleData:");
    Ipv4Address currentNode = GetNodeAddress();
    Ipv4Address requestedContent = header->GetRequestedContent();
    Ipv4Address requesterId = header->GetRequesterId();
    uint32_t requesterCommunityId = header->GetRequesterCommunityId();
    Ipv4Address destinationId = header->GetDestination();
    uint32_t interestBroadcastId = header->GetInterestBroadcastId();
    Ipv4Address bestBorderNode = header->GetBestBorderNodeId();
    NS_LOG_INFO("TUYHOA bestBorderNode "<<bestBorderNode);
    
    InterestEntry entry(requesterId, interestBroadcastId, requestedContent);
    if ( !(requesterId.IsEqual(Ipv4Address("1.0.0.1"))) || interestBroadcastId!=0)
    {
        NS_LOG_INFO("HHHHHHHHHHHHHHHHHHHHHH");
        NS_LOG_INFO(""<<requesterId);
        NS_LOG_INFO(""<<interestBroadcastId);
        
    }
    if (! (m_seenDataManager->Exist(entry)) && !(requesterId.IsEqual(currentNode)) )
    {
        global_count_data++;
        m_seenDataManager->Insert(entry);
        NS_LOG_INFO("NRL");
        NS_LOG_INFO("requester ID: "<<requesterId);
        NS_LOG_INFO("broadcast ID: "<<interestBroadcastId);
        NS_LOG_INFO("requested content: "<<requestedContent);
        NS_LOG_INFO("global_count_data: "<<global_count_data);
        NS_LOG_INFO("current node: "<<currentNode);
    }
    
    
   
    /*
        Algorithm:
        If DATA packet is destined to me:
            if header->requesterId is me: 
                SUCCESS
            else //either a border node or intermediate node that some node social-tie roward
                Turn into DATA social tie routing towards requesterId
        Else:
            If this DATA packet not in my m_pendingDataResponse:
                Turn into DATA social tie routing by by saving
                (requesterId, requesterId, content_requested, broadcast_id, requesterCommunityId, foreignDestinationId)
                into m_pendingDataResponse
    */  
    if ( currentNode.IsEqual(destinationId) )
    {
        if ( currentNode.IsEqual(requesterId) )
        {
            if (m_firstSuccess == false) {
                NS_LOG_INFO ("SUCCESS FIRST");
                m_firstSuccess = true;
            } else {
                NS_LOG_INFO("SUCCESS SECOND");
            }
            NS_LOG_INFO ("Requester node "<<currentNode<<" receives requested content "<<
                         requestedContent <<" from node: "<<header->GetSource());
            NS_LOG_INFO ("Time now " << Simulator::Now ().GetSeconds ());
            NS_LOG_INFO ("Total Hello packets: "<<global_count_hello);
            NS_LOG_INFO ("Total Interest packets: "<<global_count_interest);
            NS_LOG_INFO ("Total Data packets: "<<global_count_data);
        }
        else if ( currentNode.IsEqual(bestBorderNode) )
        {
            NS_LOG_INFO("I am the border node to relay DATA to original requester community");
            NS_LOG_INFO("Save pending DATA response entry to m_pendingDataResponse");
            PendingResponseEntry entry(requesterId, requesterId, requestedContent,
                                       interestBroadcastId, Ipv4Address("0.0.0.0"), requesterCommunityId, Ipv4Address("0.0.0.0"));
            m_pendingDataResponse->push_back(entry);
        }
		//This next else if is important. A node in community2
		//might inject DATA packet to community 1 so it sets 
		//destination address to this node.
		//Thus, this node may not be requester or border node.
		//After this node has the packet, we want this node
		//to social tie the DATA packet to the requester.
		else if (m_communityId == requesterCommunityId)
		{
			PendingResponseEntry entry(requesterId, requesterId, requestedContent, interestBroadcastId, Ipv4Address("0.0.0.0"), requesterCommunityId,
									Ipv4Address("0.0.0.0"));
			 
			if (!CheckIfPendingResponseExist(m_pendingDataResponse,entry))
			{
				NS_LOG_INFO("LONGLONG2");
				m_pendingDataResponse->push_back(entry);
				NS_LOG_INFO("Save pending DATA response entry to m_pendingDataResponse");
			}
	
		}
    }
    else
    {
        NS_LOG_INFO("Current node is "<<currentNode<<" destination is "<<destinationId);
        
        PendingResponseEntry entry(requesterId, destinationId, requestedContent,
                                   interestBroadcastId, Ipv4Address("0.0.0.0"), requesterCommunityId, Ipv4Address("0.0.0.0"));
        if (!CheckIfPendingResponseExist(m_pendingDataResponse,entry))
        {
            m_pendingDataResponse->push_back(entry);
            NS_LOG_INFO("Save pending DATA response entry to m_pendingDataResponse");
        }
    }  
}


void
SocialNetwork::HandleDigest(PktHeader *header)
{
    Ipv4Address currentNode = GetNodeAddress();
    if ( currentNode.IsEqual(header->GetDestination()) ) { //I am the destination of this digest packet?
        uint32_t contentArraySize = header->GetContentArraySize();
        ContentOwnerEntry *contentArray = (ContentOwnerEntry *) (header->GetContentArray());            

        NS_LOG_INFO("Encounter node - content array size: "<<contentArraySize);
        NS_LOG_INFO("Encounter node - content: ");
        //PrintAllContent(contentArray, contentArraySize);
        //NS_LOG_INFO("This node content before merge: ");
        PrintAllContent(m_contentManager->GetContentArray(), m_contentManager->GetContentArraySize());
        
        m_contentManager->Merge(contentArray, contentArraySize);
                    
        //NS_LOG_INFO("This node content after merge: ");
        //PrintAllContent(m_contentManager->GetContentArray(), m_contentManager->GetContentArraySize());
    }
}

void
SocialNetwork::HandleHello(PktHeader *header)
{
    global_count_hello++;
    Ipv4Address currentAddress = GetNodeAddress();
    Ipv4Address encounterAddress = header->GetSource();
    uint32_t encounterCommunityId = header->GetCommunityId();
    
    NS_LOG_INFO(""<<currentAddress<<" is in community "<<m_communityId);
    NS_LOG_INFO(""<<encounterAddress<<" is in community "<<encounterCommunityId);
    if (m_communityId != encounterCommunityId)
    {
        NS_LOG_INFO("INTER-COMMUNITY MEETING");
        m_meetForeign = true;
    }
    
    if ( !(m_foreignCommunityManager->Exist(encounterCommunityId)) && 
         encounterCommunityId != m_communityId )
    {
        m_foreignCommunityManager->Insert(encounterCommunityId);
    }
    
    Peer encounterNode = {header->GetSource(), header->GetCommunityId()};

    SocialTableEntry *socialTableEntry = (SocialTableEntry *) (header->GetSocialTieTable());
    //NS_LOG_INFO("RECEIVE: Address of social tie table: "<<header->GetSocialTieTable());
    //NS_LOG_INFO("Source address: "<<header->GetSource());
    //NS_LOG_INFO("Time: "<<Simulator::Now ().GetSeconds ());
    //NS_LOG_INFO("size: "<<header->GetSocialTieTableSize());

    // Update and merge social-tie table.
    m_relationship->UpdateAndMergeSocialTable(encounterNode,
                                           Simulator::Now(),
                                           socialTableEntry,
                                           header->GetSocialTieTableSize());
    //m_relationship->PrintSocialTable();
    
    DecideWhetherToSendContentNameDigest(header);
   

    //switch fringe node set with encountered noed
    //fringeNodeSet.UpdateSocialTable((uint32_t *)(m_relationship->GetSocialTableAddress()),
      //                              m_relationship->GetSocialTableSize(),
        //                            (uint32_t *)(socialTableEntry),
          //                          header->GetSocialTieTableSize());
    fringeNodeSet.clear();
    qhs.UpdateFringeNodeSet((m_relationship->GetSocialTableAddress()),
                                    m_relationship->GetSocialTableSize(), fringeNodeSet, m_communityId); 
    ///////////////////// Send out pending response if any condition matches ///////////////////////
    
    // 1) Take care of m_pendingDataResponse
    HandlePendingDataResponse(header);
    
    // 2) Take care of m_pending_interest_response
    // These are pending interests that do not know which node owns the requested content.
    HandlePendingInterestResponse(header);
    
    // 3) Take care of m_pending_content_dest_node_response
    // These are pending interests that already know which node is a content provider.
    HandlePendingContentDestNodeResponse(header);
     
}

void
SocialNetwork::SendPacket(PktHeader header)
{
    Ptr<Packet> p = Create<Packet> (m_size);
    p->AddHeader(header);
    
    // call to the trace sinks before the packet is actually sent,
    // so that tags added to the packet can be sent as well
    m_txTrace (p);
    m_socket->Send (p);
}

void
SocialNetwork::DecideWhetherToSendContentNameDigest(PktHeader *header)
{
    /*
    Algorithm:
    If encounter is in the same community:
        If he has higher centrality than me:
            Send content name digest to him
        End if
    Else
        // Encounter is in a foreign community J:
        If last_foreign_encounter_node_J == NULL:
            Send content name digest to him
            Update last_foreign_encounter_node_J
        Else
            If encounter has higher centrality than last_foreign_encounter_node_J:
                Send content name digest to him
                Update last_foreign_encounter_node_J
            End if
        End if
    End if
    */

    Ipv4Address currentNode = GetNodeAddress();
    Ipv4Address encounterNode = header->GetSource();
    uint32_t encounterCommunityId = header->GetCommunityId();
    
    if (encounterCommunityId == m_communityId)
    {    
        NS_LOG_INFO("SAME COMMUNITY");
        Ipv4Address higherCentralityNode = m_relationship->GetHigherCentralityNode(currentNode, encounterNode);
        if (higherCentralityNode.IsEqual(encounterNode))
        {
            //Unicast content name digest packet to encounter node
            NS_LOG_INFO(""<<currentNode<<" sends content name digest to node "<<encounterNode);
            PktHeader *header = CreateDigestPacketHeader(encounterNode);
            SendPacket(*header);
        }
    }
    else
    {
        NS_LOG_INFO("DIFFERENT COMMUNITY");
        if ( m_last_foreign_encounter_node[encounterCommunityId].IsEqual(Ipv4Address("0.0.0.0")) )
        {
            NS_LOG_INFO(""<<currentNode<<" sends content name digest to node "<<encounterNode);
            PktHeader *header = CreateDigestPacketHeader(encounterNode);
            SendPacket(*header);
            m_last_foreign_encounter_node[encounterCommunityId] = encounterNode;
        }
        else
        {
            Ipv4Address higherCentralityNode =
                m_relationship->GetHigherCentralityNode(encounterNode, m_last_foreign_encounter_node[encounterCommunityId]);
            if ( higherCentralityNode.IsEqual(encounterNode) )
            {
                NS_LOG_INFO(""<<currentNode<<" sends content name digest to node "<<encounterNode);
                PktHeader *header = CreateDigestPacketHeader(encounterNode);
                SendPacket(*header);
                m_last_foreign_encounter_node[encounterCommunityId] = encounterNode;
            }
        }
    }
}

void
SocialNetwork::HandlePendingContentDestNodeResponse(PktHeader *header)
{
    Ipv4Address currentNode = GetNodeAddress();
    Ipv4Address encounterNode = header->GetSource();
    uint32_t encounterCommunityId = header->GetCommunityId();
    //If encounter node (sourceAddr) matches content provider ID in m_pending_content_dest_node_response entry
    //then unicast interest immediately. Otherwise, continue following social-tie routing towards content provider.
    
    /*
    Algorithm:
    For each response in m_pending_content_dest_node_response:
        If encounter is response->destinationId:
            //destinationId here could be either border node or true content provider
            //but it doesn't matter. We just set foreignDestinationId in packet header
            //to response->foreignDestinationId
            Unicast INTEREST packet to encounter
        Else:
            If encounterCommunityId == m_communityId:
                If encounter has higher social tie toward response->destinationId than me:
                    If last_relay_encounter[m_communityId] == NULL:
                        Unicast INTEREST packet to encounter
                        Update last_relay_encounter[m_communityId] to encounter
                    Else:
                        If encounter has higher social tie toward content provider than last_relay_encounter[m_communityId]:
                            Unicast INTEREST packet to encounter
                            Update last_relay_encounter[m_communityId] to encounter
            //No need to consider meet a node in different community.
            //Since the current node has clear target. Either route it to border node or to a content provider
            //in the same community.
    */
    
    NS_LOG_INFO("");
    NS_LOG_INFO("Inside HandlePendingContentDestNodeResponse");
    NS_LOG_INFO("m_pending_content_dest_node_response size: "<<m_pending_content_dest_node_response->size());
    
    for (vector<PendingResponseEntry>::iterator it = m_pending_content_dest_node_response->begin();
         it != m_pending_content_dest_node_response->end(); ++it)
    {
        if ( (it->destinationId).IsEqual(encounterNode) ) //encounter node is the content provider
        {
            //Unicast INTEREST packet
            //global_count_interest++;
            PktHeader *header = CreateInterestPacketHeader(it->requesterId, encounterNode,
                                it->destinationId, it->contentRequested, it->broadcastId,
                                it->requesterCommunityId, it->foreignDestinationId, it->bestBorderNodeId);
            SendPacket(*header);
            
			NS_LOG_INFO(currentNode<<"PHUYEN: "<<Simulator::Now().GetSeconds());
            NS_LOG_INFO("Send INTEREST packet with content ("<<it->contentRequested<<") to "<<encounterNode);
            if ( !((it->foreignDestinationId).IsEqual(Ipv4Address("0.0.0.0"))) )
            {
                NS_LOG_INFO("The destination is the border node: "<<it->destinationId);
                NS_LOG_INFO("The foreign content provider is: "<<it->foreignDestinationId);
            }
            else
            {
                NS_LOG_INFO("The destination is the content provider: "<<it->destinationId);
            }
        }
        else //encounter node is not the content provider, use social tie routing to propagate INTEREST packet
        {
            if (encounterCommunityId == m_communityId) //same community
            {
                Ipv4Address higherSocialTieNode =
                        m_relationship->GetHigherSocialTie(currentNode, encounterNode, it->destinationId);
                if (higherSocialTieNode.IsEqual(encounterNode))
                {
                    if ( (it->lastRelayNode[m_communityId]).IsEqual(Ipv4Address("0.0.0.0")) ) //test if lastRelayNode is NULL
                    {
                        it->lastRelayNode[m_communityId] = encounterNode;
                        
                        //Unicast INTEREST packet
                        //global_count_interest++;
                        PktHeader *header = CreateInterestPacketHeader(it->requesterId, encounterNode,
                                    it->destinationId, it->contentRequested, it->broadcastId,
                                    it->requesterCommunityId, it->foreignDestinationId, it->bestBorderNodeId);
                        SendPacket(*header);

						NS_LOG_INFO(currentNode<<"PHUYEN: "<<Simulator::Now().GetSeconds());
                
                        NS_LOG_INFO("Send INTEREST packet with content ("<<it->contentRequested<<") to "<<encounterNode);
                        NS_LOG_INFO("The target destination is: "<<it->destinationId);
                    }
                    else
                    {
                        Ipv4Address higherSocialTieNode2 =
                                m_relationship->GetHigherSocialTie(encounterNode, it->lastRelayNode[m_communityId], it->destinationId);
                        if ( higherSocialTieNode2.IsEqual(encounterNode) )
                        {
                            it->lastRelayNode[m_communityId] = encounterNode;
                            
                            //Unicast INTEREST packet
                            //global_count_interest++;
                            PktHeader *header = CreateInterestPacketHeader(it->requesterId, encounterNode,
                                    it->destinationId, it->contentRequested, it->broadcastId,
                                    it->requesterCommunityId, it->foreignDestinationId, it->bestBorderNodeId);
                            SendPacket(*header);
                    		NS_LOG_INFO(currentNode<<"PHUYEN: "<<Simulator::Now().GetSeconds());

                            NS_LOG_INFO("Send INTEREST packet with content ("<<it->contentRequested<<") to "<<encounterNode);
                            NS_LOG_INFO("The target content provider is: "<<it->destinationId);
                        }
                    }
                }
            }
        }
    }
}

void
SocialNetwork::HandlePendingInterestResponse(PktHeader *header)
{
    Ipv4Address currentNode = GetNodeAddress();
    Ipv4Address encounterNode = header->GetSource();
    uint32_t encounterCommunityId = header->GetCommunityId();
    
    if (m_meetForeign == true)
    {
        for (vector<PendingResponseEntry>::iterator it = m_pending_interest_response->begin();
            it != m_pending_interest_response->end(); ++it)
        {
            if (encounterCommunityId == m_communityId) //same community
            {
                NS_LOG_INFO("YOULU2");
                PktHeader *header = CreateInterestPacketHeader(it->requesterId, encounterNode,
                                                        Ipv4Address("0.0.0.0"), it->contentRequested,
                                                        it->broadcastId, it->requesterCommunityId, Ipv4Address("0.0.0.0"),
                                                        Ipv4Address("0.0.0.0"));
                SendPacket(*header);
            }
        }
    }
    // Take care of m_pending_interest_response
    // These are pending interests that do not know which node owns the requested content.
    /*
    Algorithm:
    For each response in m_pending_interest_response:
        If encounter and I are in the same community:
            If encounter has higher social level than me:
                If last_data_relay_encounter[m_communityId] == NULL:
                    Unicast INTEREST packet to encounter
                    Update last_relay_encounter[m_communityId] to encounter
                Else:
                    If encounter has higher social level than last_relay_encounter[m_communityId]:
                        Unicast INTEREST packet to encounter
                        Update last_relay_encounter[m_communityId] to encounter
        Else:
            If last_data_relay_encounter[encounterCommunityId] == NULL:
                Unicast INTEREST packet to encounter
                Update last_relay_encounter[encounterCommunityId] to encounter
            Else:
                If encounter has higher social level than last_relay_encounter[encounterCommunityId]:
                    Unicast INTEREST packet to encounter
                    Update last_relay_encounter[encounterCommunityId] to encounter
                
                
    Note: Above shows my GREEDY approach to disseminate INTEREST packets to different communities
          at the border node level.
    */
    
    NS_LOG_INFO("");
    NS_LOG_INFO("Inside HandlePendingInterestResponse");
    NS_LOG_INFO("m_pending_interest_response size: "<<m_pending_interest_response->size());
    for (vector<PendingResponseEntry>::iterator it = m_pending_interest_response->begin();
         it != m_pending_interest_response->end(); ++it)
    {
        if (encounterCommunityId == m_communityId) //same community
        {
            Ipv4Address higherSocialLevelNode = m_relationship->GetHigherSocialLevel(currentNode, encounterNode);
            if ( higherSocialLevelNode.IsEqual(encounterNode) )
            {
                if ( (it->lastRelayNode[m_communityId]).IsEqual(Ipv4Address("0.0.0.0")) ) //test if lastRelayNode is NULL
                {
                    it->lastRelayNode[m_communityId] = encounterNode;
                    //Unicast INTEREST packet
                    //global_count_interest++;
                    PktHeader *header = CreateInterestPacketHeader(it->requesterId, encounterNode,
                                                    Ipv4Address("0.0.0.0"), it->contentRequested,
                                                    it->broadcastId, it->requesterCommunityId, it->foreignDestinationId,
                                                    it->bestBorderNodeId);
                    SendPacket(*header);
					NS_LOG_INFO(currentNode<<"PHUYEN: "<<Simulator::Now().GetSeconds());
                    NS_LOG_INFO("Send INTEREST packet with content ("<<it->contentRequested<<") to "<<encounterNode);
                }
                else
                {
                    Ipv4Address higherSocialLevelNode2 =
                            m_relationship->GetHigherSocialLevel(encounterNode, it->lastRelayNode[m_communityId]);
                    if ( higherSocialLevelNode2.IsEqual(encounterNode) )
                    {
                        it->lastRelayNode[m_communityId] = encounterNode;
                    
                        //Unicast INTEREST packet
                        //global_count_interest++;
                        PktHeader *header = CreateInterestPacketHeader(it->requesterId, encounterNode,
                                                        Ipv4Address("0.0.0.0"), it->contentRequested,
                                                        it->broadcastId, it->requesterCommunityId, it->foreignDestinationId,
                                                        it->bestBorderNodeId);
                        SendPacket(*header);
						NS_LOG_INFO(currentNode<<"PHUYEN: "<<Simulator::Now().GetSeconds());
                        NS_LOG_INFO("Send INTEREST packet with content ("<<it->contentRequested<<") to "<<encounterNode);
                    }
                }
            }
        }
        else
        {
            if ( (it->lastRelayNode[encounterCommunityId]).IsEqual(Ipv4Address("0.0.0.0")) )
            {
                it->lastRelayNode[encounterCommunityId] = encounterNode;
                //Unicast INTEREST packet
                //global_count_interest++;
                PktHeader *header = CreateInterestPacketHeader(it->requesterId, encounterNode,
                                                Ipv4Address("0.0.0.0"), it->contentRequested,
                                                it->broadcastId, it->requesterCommunityId, it->foreignDestinationId,
                                                it->bestBorderNodeId);
                SendPacket(*header);
				NS_LOG_INFO(currentNode<<"PHUYEN: "<<Simulator::Now().GetSeconds());
                NS_LOG_INFO("Send INTEREST packet with content ("<<it->contentRequested<<") to "<<encounterNode);
            }
            else
            {
                Ipv4Address higherSocialLevelNode =
                            m_relationship->GetHigherSocialLevel(encounterNode, it->lastRelayNode[encounterCommunityId]);
                if ( higherSocialLevelNode.IsEqual(encounterNode) )
                {
                    it->lastRelayNode[encounterCommunityId] = encounterNode;
                    //Unicast INTEREST packet
                    //global_count_interest++;
                    PktHeader *header = CreateInterestPacketHeader(it->requesterId, encounterNode,
                                                    Ipv4Address("0.0.0.0"), it->contentRequested,
                                                    it->broadcastId, it->requesterCommunityId, it->foreignDestinationId,
                                                    it->bestBorderNodeId);
                    SendPacket(*header);
					NS_LOG_INFO(currentNode<<"PHUYEN: "<<Simulator::Now().GetSeconds());
                    NS_LOG_INFO("Send INTEREST packet with content ("<<it->contentRequested<<") to "<<encounterNode);
                }
            }
        }
    }
}


void
SocialNetwork::HandlePendingDataResponse(PktHeader *header)
{
    Ipv4Address currentNodeAddress = GetNodeAddress();
    Ipv4Address encounterNode = header->GetSource();
    uint32_t encounterCommunityId = header->GetCommunityId();
    
    //Take care of m_pendingDataResponse.
    /*
    Algorithm:
    For each response in m_pendingDataResponse:
        If the encounter node is the destination node the DATA packet should be destined to.
            Unicast DATA packet to the encounter
        Else:
            If encounter node and I are in the same community: //either route to final destination or to border node
                If encounter node has higher social tie toward response->destination than me:
                    If last_relay_encounter[m_communityId] == NULL:
                        Unicast DATA packet to encounter node
                        Update last_relay_encounter[m_communityId] to encounter
                    Else:
                        If encounter node has higher social tie toward response->destination than last_relay_encounter[m_communityId]:
                            Unicast DATA packet to encounter node
                            Update last_relay_encounter[m_communityId] to encounter
            Else:
                If response->requesterCommunityId and encounter node are in the same community:
                    If last_relay_encounter[response->requesterCommunityId] == NULL:
                        Unicast DATA packet to encounter node
                        Update last_relay_encounter[response->requesterCommunityId] to encounter
                    Else:
                        If encounter node has higher social tie
                                toward response->requesterId than last_relay_encounter[response->requesterCommunityId]:
                            Unicast DATA packet to encounter node
                            Update last_relay_encounter[response->requesterCommunityId] to encounter
    */
    
    NS_LOG_INFO("");
    NS_LOG_INFO("Inside HandlePendingDataResponse");
    NS_LOG_INFO("m_pendingDataResponse size: "<<m_pendingDataResponse->size());
    //vector<uint32_t> remove_index; //keep track of element index that needs to remove from m_pendingDataResponse
        //The index corresponds to pending DATA packet that has been sent out.
        
    // Note: We never remove the pending entry even after we already sent it since
    // the next time we might encounter a node with a stronger social tie toward requester
    // then last time, we will send out DATA again.
    for (vector<PendingResponseEntry>::iterator it = m_pendingDataResponse->begin();
         it != m_pendingDataResponse->end(); ++it)
    {
        if ( (it->requesterId).IsEqual(encounterNode) ) //the encounter node is the requester node
        {
            //Send DATA packet
            PktHeader *header = CreateDataPacketHeader(encounterNode, it->requesterId,
                                        it->contentRequested, it->requesterCommunityId,
                                        Ipv4Address("0.0.0.0"), it->bestBorderNodeId, it->broadcastId);
            SendPacket(*header);
            NS_LOG_INFO("Encounter node is the original requester node.");
            NS_LOG_INFO("Send DATA packet with content ("<<it->contentRequested<<") to "<<encounterNode);
        }
        else if ( (it->destinationId).IsEqual(encounterNode) )
        {
            //Send DATA packet
            NS_LOG_INFO("Meet destination directly.");
            PktHeader *header = CreateDataPacketHeader(encounterNode, it->requesterId,
                                        it->contentRequested, it->requesterCommunityId,
                                        it->foreignDestinationId, it->bestBorderNodeId,
                                        it->broadcastId);
            SendPacket(*header);
            NS_LOG_INFO("Send DATA packet with content ("<<it->contentRequested<<") to "<<encounterNode);
        }
        else //encounter node is not the requester or destination node, use social tie routing to propagate DATA packet
        {
            if (encounterCommunityId == m_communityId) //same community
            {   
                NS_LOG_INFO("Encounter and I are in same community");
                Ipv4Address higherSocialTieNode=
                        m_relationship->GetHigherSocialTie(currentNodeAddress, encounterNode, it->destinationId);
                if (higherSocialTieNode.IsEqual(encounterNode))
                {
                    if ( (it->lastRelayNode[m_communityId]).IsEqual(Ipv4Address("0.0.0.0")) ) //test if lastRelayNode is NULL
                    {
                        it->lastRelayNode[m_communityId] = encounterNode;
                        //Send DATA packet
                        PktHeader *header = CreateDataPacketHeader(encounterNode,
                                it->requesterId, it->contentRequested, it->requesterCommunityId,
                                it->foreignDestinationId, it->bestBorderNodeId, it->broadcastId);
                        SendPacket(*header);
                        NS_LOG_INFO("Send DATA packet with content ("<<it->contentRequested<<") to "<<encounterNode);
                    }
                    else
                    {
                        Ipv4Address higherSocialTieNode2 =
                            m_relationship->GetHigherSocialTie(encounterNode, it->lastRelayNode[m_communityId], it->destinationId);
                        if ( higherSocialTieNode2.IsEqual(encounterNode) )
                        {
                            it->lastRelayNode[m_communityId] = encounterNode;
                            //Send DATA packet
                            PktHeader *header = CreateDataPacketHeader(encounterNode, it->requesterId,
                                        it->contentRequested, it->requesterCommunityId, it->foreignDestinationId,
                                        it->bestBorderNodeId, it->broadcastId);
                            SendPacket(*header);
                            NS_LOG_INFO("Send DATA packet with content ("<<it->contentRequested<<") to "<<encounterNode);
                        }
                    }
                }
            }
            else //encounter and I are in different communities
            {
                if ( it->requesterCommunityId == encounterCommunityId ) //if the original requester and the encounter are in same community
                {
                    NS_LOG_INFO("Requester and encounter are in same community");
					if (currentNodeAddress.IsEqual(Ipv4Address("1.0.0.45")))
					{
						NS_LOG_INFO("HERE");
					}
					NS_LOG_INFO("VINCE");
                    if ( (it->lastRelayNode[it->requesterCommunityId]).IsEqual(Ipv4Address("0.0.0.0")) ) //test if lastRelayNode is NULL
                    {
                        it->lastRelayNode[it->requesterCommunityId] = encounterNode;
                        //Send DATA packet
                        PktHeader *header = CreateDataPacketHeader(encounterNode, it->requesterId,
                                    it->contentRequested, it->requesterCommunityId,
                                    Ipv4Address("0.0.0.0"), it->bestBorderNodeId, it->broadcastId);
                                                         //Since we already cross the border, the last parameter will be 0.0.0.0
                        SendPacket(*header);
                        NS_LOG_INFO("Send DATA packet with content ("<<it->contentRequested<<") to "<<encounterNode);
						NS_LOG_INFO("VINCE1");

						if (currentNodeAddress.IsEqual(Ipv4Address("1.0.0.45")))
						{
							NS_LOG_INFO("HERE1");
						}

                    }
                    else
                    {
                        Ipv4Address higherSocialTieNode2 =
                            m_relationship->GetHigherSocialTie(encounterNode, it->lastRelayNode[it->requesterCommunityId], it->destinationId);
                        if ( higherSocialTieNode2.IsEqual(encounterNode) )
                        {
                            it->lastRelayNode[it->requesterCommunityId] = encounterNode;
                            //Send DATA packet
                            PktHeader *header = CreateDataPacketHeader(encounterNode, it->requesterId,
                                        it->contentRequested, it->requesterCommunityId,
                                        it->foreignDestinationId, it->bestBorderNodeId, it->broadcastId);
                            SendPacket(*header);
                            NS_LOG_INFO("Send DATA packet with content ("<<it->contentRequested<<") to "<<encounterNode);
							NS_LOG_INFO("VINCE2");
							if (currentNodeAddress.IsEqual(Ipv4Address("1.0.0.45")))
							{
								NS_LOG_INFO("HERE");
							}

                        }
                    }
                }
            }
            
        }
        
    } //end of for loop
}

void
SocialNetwork::PrintAllContent(ContentOwnerEntry *array, uint32_t size)
{
    for (uint32_t i=0;i<size;++i)
    {
        NS_LOG_INFO("Content name:"<< array[i].contentName<<" Content owner:"<< array[i].contentOwner );
    }
}

void
SocialNetwork::HandleInterest(PktHeader *header)
{
    NS_LOG_INFO("Inside HandleInterest");
    Ipv4Address currentNodeAddress = GetNodeAddress();
    Ipv4Address encounterNode = header->GetSource();
    
    // 1) If receiving the same interest, ignore the interest!
    //    Each interest is uniquely identified by <original requester ID, broadcast_id>
    //    These 2 fields are found in packet header.
    //    We don't use content name as unique identifier since it might be the case that
    //    after receiving the DATA, the original requester might want to request the same content again.
    
    Ipv4Address requesterId = header->GetRequesterId();
    uint32_t requesterCommunityId = header->GetRequesterCommunityId();
    Ipv4Address contentProviderId = header->GetContentProviderId();
    uint32_t broadcastId = header->GetInterestBroadcastId();
    Ipv4Address requestedContent = header->GetRequestedContent();
    Ipv4Address foreignDestinationId = header->GetForeignDestinationId();
    Ipv4Address borderNodeId = header->GetBestBorderNodeId();
    NS_LOG_INFO("Packet type: "<<GetPacketTypeName(header->GetPacketType()));
    NS_LOG_INFO("Interest packet - requesterId: "<<requesterId);
    NS_LOG_INFO("Interest packet - requesterCommunityId: "<<requesterCommunityId);
    NS_LOG_INFO("Interest packet - contentProviderId: "<<contentProviderId);
    NS_LOG_INFO("Interest packet - broadcastId: "<<broadcastId);
    NS_LOG_INFO("Interest packet - requestedContent: "<<requestedContent);
    NS_LOG_INFO("Interest packet - foreignDestinationId: "<<foreignDestinationId);
    NS_LOG_INFO("Interest packet - borderNodeId: "<<borderNodeId);
    NS_LOG_INFO("Interest packet - destinationNodeId: "<<header->GetDestination());
    //NS_LOG_INFO("This node has: "<<GetAllContentInOneString(m_contentManager->GetContentArray(),
    //                                                        m_contentManager->GetContentArraySize()));
    InterestEntry interestEntry(requesterId, broadcastId, requestedContent);
    
    /*
    Algorithm: (inter community routing)
        When receiving an Interest packet
        If not see this Interest before:
            Insert the interest into my InterestManager
        If I am a destination:
            If foreign node has the content: //I must be the border node => need to relay the INTEREST to foreign content provider
                Turn into INTEREST social-tie routing to foreign content provider by saving
                (requesterId, foreignDestinationId, requestedContent, broadcastId, Ipv4Address("0.0.0.0")) into
                m_pending_content_dest_node_response. Note that the last field indicates that this is final routing. No more border relay.
            Else: //I have the content
                If requester is in the same community with me:
                    Turn into DATA social-tie routing to destination by saving
                    (requesterId, requesterId, requestedContent, broadcast_id, Ipv4Address("0.0.0.0")) into m_pendingDataResponse
                Else:
                    If I am best border node:
                        Turn into DATA social-tie routing to the foreign requester directly
                    Else if best border node not NULL:
                        Turn into DATA social-tie routing to best border node by saving
                        (requesterId, bestBorderNode, requestedContent, broadcast_id, requesterId) into m_pendingDataResponse
                    End if
                End if
            End if
        Else:
            Check my local content table
            If I have the content:
                If encounter node is the requester:
                    send DATA packet to encounter node
                Else:
                    If requester is in the same community with me:
                        Turn into DATA social-tie routing to destination by saving
                        (requesterId, requesterId, requestedContent, broadcast_id, Ipv4Address("0.0.0.0")) into m_pendingDataResponse
                    Else:
                        If I am best border node:
                            Turn into DATA social-tie routing to the foreign requester directly
                        Else if best border node not NULL:
                            Turn into DATA social-tie routing to best border node by saving
                            (requesterId, bestBorderNode, requestedContent, broadcast_id, requesterId) into m_pendingDataResponse
                        End if
                    End if
                End if
            Else:
                If packet header tells me some node has the content (GetContentProviderId returns a valid address): 
                    //GetContentProviderId can return a local address or a foreign address
                    //If it is a local address, set last parameter in PendingEntryResponse to 0.0.0.0
                    //Otherwise, set last parameter in PendingEntryResponse to correct foreign address
                    //Not important. Just put foreignDestinationId into the last parameter. Its value can be either
                    0.0.0.0 or a valid foreign address.
                    Turn into INTEREST social-tie routing by saving
                    (requesterId, GetContentProviderId, broadcastId, contentRequested, foreignDestinationId)
                    into m_pending_content_dest_node_response
                Else If my table tells me some other node has the content:
                    If other node is a foreign node in community J:
                        If I am best border node:
                            Turn into INTEREST social-tie routing to foreign content provider by saving
                            (requesterId, contentOwner, requestedContent, broadcastId, Ipv4Address("0.0.0.0"))
                            into m_pending_content_dest_node_response
                        Else best border node not NULL:
                            Turn into INTEREST social-tie routing to best border node by saving
                            (requesterId, bestBorderNode, requestedContent, broadcastId, contentOwner)
                            into m_pending_content_dest_node_response.
                    Else:
                        Turn into INTEREST social-tie routing by saving
                        (requesterId, contentOwnerId, broadcastId, contentRequested, Ipv4Address("0.0.0.0"))
                        into m_pending_content_dest_node_response
                        //NOTE: in HandleHello, a node will check its m_pending_content_dest_node_response
                        //For each pending X there, it will check if encouter social tie toward contentDestinationNode is higher
                        //then itself ...
                    End if
                Else:
                    Turn into INTEREST social-level routing by saving
                    (requesterId, broadcastId, contentRequested) into m_pending_interest_response
                    
                    If I have highest social level: //potentially to be a cluster head
                        For each known foreign community X:
                            If bestBorderNode to community X not NULL:
                                Turn into INTEREST social-tie routing to bestBorderNode
                            End if
                        End for
                    End if
                End if
            End if
        End if
                        
    */
    
    /*if (! (m_interestManager->Exist(interestEntry)) )
    {
        //first time sees this Interest packet
        m_interestManager->Insert(interestEntry);
    }*/
    Ipv4Address contentOwner = m_contentManager->GetOwnerOfContent(requestedContent);
    NS_LOG_INFO("Owner of content "<< requestedContent <<" is: "<<contentOwner);
    if (contentOwner.IsEqual(currentNodeAddress)) //I have the content
    {
        NS_LOG_INFO("LAUREN");
        if (encounterNode.IsEqual(requesterId)) {
        //encounter is the requester => no need to
        //social tie or anything. Just send DATA directly
	//Send DATA packet
        //Qiuhan: change
            vector<Ipv4Address>borderNode_t;
	    //PktHeader *header = CreateDataPacketHeader(encounterNode, requesterId,
				//requestedContent, requesterCommunityId,
				//foreignDestinationId, Ipv4Address("0.0.0.0"), broadcastId);
	    PktHeader *header = CreateDataPacketHeader(encounterNode, requesterId,
				requestedContent, requesterCommunityId,
				foreignDestinationId, borderNode_t, broadcastId);
	    SendPacket(*header);
	    NS_LOG_INFO("Send DATA packet with content ("<< requestedContent <<") to "<<encounterNode);
        }
        else //encounter is not the requester
        {
	    if (requesterCommunityId == m_communityId) //requester and content provider (me) are in the same community
	    {
                //Qiuhan: change
                vector<Ipv4Address>borderNode_t;
	        NS_LOG_INFO("Requester and content provider are in the same community");
	        PendingResponseEntry entry(requesterId, requesterId,
		     	    requestedContent, broadcastId, Ipv4Address("0.0.0.0"), requesterCommunityId, borderNode_t);
	        ProcessPendingDataResponse(entry, interestEntry); 
	    }
	    else //requester and content provider (me) are in different communities
	    {
	        NS_LOG_INFO("Requester and content provider are in different communities");
	    // Qiuhan: m_relationship->GetFringeNodeSet(requesterCommunityId), and using for loop to send interest to all these node
	    // Qiuhan: Or if the node itself exist in the fringe node set,
	    // whether will it send by itself? and not send to other fringe node
	        //Ipv4Address bestBorderNode = m_relationship->GetBestBorderNode(requesterCommunityId);
	        //NS_LOG_INFO("Best border node is: "<<bestBorderNode);
	        //NS_LOG_INFO("SAIGON "<<bestBorderNode);
                auto Nodeset = fringeNodeSet.find(requestCommunityId);
	        //m_relationship->PrintSocialTable();
		if (Nodeset != fringeNodeSet.end())
		{
		  for(auto node: Nodeset) {
	              if (node.IsEqual(currentNodeAddress))
	              {
		        //social-tie DATA directly to foreign requester
                        vector<Ipv4Address>borderNode_t;
		        PendingResponseEntry entry(requesterId, requesterId,
	    		    requestedContent, broadcastId, Ipv4Address("0.0.0.0"), 
			    requesterCommunityId, borderNode_t);
		        ProcessPendingDataResponse(entry, interestEntry); 
	              }
	              else if ( !(node.IsEqual(Ipv4Address("0.0.0.0"))) )
	              {
		        PendingResponseEntry entry(requesterId, node,
		         	    requestedContent, broadcastId, requesterId, requesterCommunityId, Nodeset);
		        ProcessPendingDataResponse(entry, interestEntry);  
	              }
                  }
		}
		else
		{
		    for(auto Set: fringeNodeSet) {
		        for(auto node: Set) {
			    
		            PendingResponseEntry entry(requesterId, node,
		         	    requestedContent, broadcastId, requesterId, requesterCommunityId, Set);
		            ProcessPendingDataResponse(entry, interestEntry);  
			}
                    }
		}
	    }
        }
    }
    else {    
	// Qiuhan: I don't have content
        if ( currentNodeAddress.IsEqual(header->GetDestination()) ) //I am the destination
        //doesn't mean I am the border node or content provider for sure
        //a node Y can unicast interest packet to node X that has higher social tie toward content provider
        //Thus, X is the destination node Y sends to, but X is not content provider
        {
      	    auto itr = find(borderNodeId.begin(), borderNodeId.end(), currentNodeAddress); 
            if ( !(foreignDestinationId.IsEqual(Ipv4Address("0.0.0.0"))) &&  //foreign node has the content
		 itr != borderNodeId.end())
                 //currentNodeAddress.IsEqual(borderNodeId) )
            {
            //Inside here meaning I am the border node.
        
            // example: a node in community 1 requests a content that community 2 owns.
            // when the clusterhead in community 1 receives the INTEREST packet,
            // if it knows community 2 has the content, it will social-tie INTEREST packet to border
            // node. Here's the border node receives and social-tie the INTEREST packet to
            // content provider in community 2.
		vector<Ipv4Address>borderNode_t;
                NS_LOG_INFO("Foreign node has the content. Save pending INTEREST packet to route to foreign content provider.");
                PendingResponseEntry entry(requesterId, foreignDestinationId,
                                requestedContent, broadcastId, Ipv4Address("0.0.0.0"), requesterCommunityId, borderNode_t);
		ProcessPendingContent(entry, interestEntry);
                
            } 
            else //I am neither the border node nor the content provider. I have high social-tie towards
             //destination. I need to relay this Interest packet => 2 cases: I know who is the
             //content provider or I do not know.
            {
                NS_LOG_INFO("XIAO");
            // Qiuhan: What is the function of borderNodeId equal to 0.0.0.0
                //if (borderNodeId.IsEqual(Ipv4Address("0.0.0.0")) &&
		if (borderNodeId.empty() &&
                    foreignDestinationId.IsEqual(Ipv4Address("0.0.0.0")))
                {
                    vector<Ipv4Address>borderNode_t;
                    PendingResponseEntry entry(requesterId, Ipv4Address("255.255.255.255"),
                                               requestedContent, broadcastId, Ipv4Address("0.0.0.0"),
                                               requesterCommunityId, borderNode_t);
		    ProcessPendingInterest(entry, interestEntry);
                    
                    if ( !(contentProviderId.IsEqual(Ipv4Address("0.0.0.0"))) &&
                        foreignDestinationId.IsEqual(Ipv4Address("0.0.0.0")) )
                    {
                        NS_LOG_INFO("XIAO2");
                        if (!CheckIfPendingResponseExist(m_pending_interest_response,entry))
                        {
                	    vector<Ipv4Address>borderNode_t;
                            PendingResponseEntry entry(requesterId, contentProviderId,
                                requestedContent, broadcastId, foreignDestinationId, 
                                requesterCommunityId, borderNode_t);
		            ProcessPendingContent(entry, interestEntry);
                           
                        }
                    }
                }
            }
        }
        else //I am not the destination of the interest packet
        {   
        
         //I do not have the content
            if ( !(contentProviderId.IsEqual(Ipv4Address("0.0.0.0"))) )
            //I am a relay node for an interest packet towards a known content provider
            {
              // Qiuhan: this part also need restructured
                //The known content provider can be in the same community or in foreign community
                vector<Ipv4Address>borderNode_t;
                PendingResponseEntry entry(requesterId, contentProviderId,
                            requestedContent, broadcastId, foreignDestinationId, 
                            requesterCommunityId, borderNode_t);
		ProcessPendingContent(entry, interestEntry);
                
            }
            else if ( !(contentOwner.IsEqual(Ipv4Address("0.0.0.0"))) ) //My content table tells me other node has a content
            {
                uint32_t content_owner_community_id = m_contentManager->GetCommunityIdOfOwnerOfContent(contentOwner);
                if (content_owner_community_id != m_communityId) //the node that has the content is in different community from me
                {
                  // Qiuhan: need restructured, maybe we can add a function called sendPacketToForeignCommunity
                    //Ipv4Address bestBorderNode = m_relationship->GetBestBorderNode(content_owner_community_id);
		    auto set = fringeNodeSet.find(content_owner_community_id);
		    if (set != fringeNodeSet.end()) {
		        for (auto node:*set) {
                        //NS_LOG_INFO("SAIGON "<<bestBorderNode);
                            if (node.IsEqual(currentNodeAddress)) //I happen to be the best border node
                            {
                		vector<Ipv4Address>borderNode_t;
                                PendingResponseEntry entry(requesterId, contentOwner,
                                        requestedContent, broadcastId, Ipv4Address("0.0.0.0"), 
					requesterCommunityId, borderNode_t);
		                ProcessPendingContent(entry, interestEntry);
                       
                            }
                            else if ( !(node.IsEqual(Ipv4Address("0.0.0.0"))) )
                            {
                                //social-tie route Interest packet to node that meets most nodes in community J
                                PendingResponseEntry entry(requesterId, node,
                                        requestedContent, broadcastId, contentOwner, requesterCommunityId, *set);
                                NS_LOG_INFO("TRANG contentOwner"<<contentOwner);
		                ProcessPendingContent(entry, interestEntry);
                        
                            }
			}
                    }
                }
                else //the node that has the content is in the same community with me
                {
                    PendingResponseEntry entry(requesterId, contentOwner,
                                requestedContent, broadcastId, Ipv4Address("0.0.0.0"), requesterCommunityId, Ipv4Address("0.0.0.0"));
		    ProcessPendingContent(entry, interestEntry);
                   
                }
            }
            else //I don't know which node has the content
            {
                //keep this interest packet for local search
                vector<Ipv4Address>borderNode_t;
                PendingResponseEntry entry(requesterId, Ipv4Address("255.255.255.255"),
                                           requestedContent, broadcastId, Ipv4Address("0.0.0.0"),
                                           requesterCommunityId, borderNode_t);
		ProcessPendingInterest(entry, interestEntry);
                // Qiuhan: Here we need to send interest to all fringe nodes in all foreign community
                //also propagate the interest to other communities if I am a clusterhead
                if ( m_relationship->HasHighestSocialLevel(currentNodeAddress, m_communityId) )
                {
                    uint32_t *foreignCommunityIds = m_foreignCommunityManager->GetCommunityArray();
                    uint32_t foreignCommunityArraySize = m_foreignCommunityManager->GetCommunityArraySize();
                    for (uint32_t i=0;i<foreignCommunityArraySize;++i)
                    {
			
                        //Ipv4Address bestBorderNode = m_relationship->GetBestBorderNode(foreignCommunityIds[i]);
                        //NS_LOG_INFO("SAIGON "<<bestBorderNode);
			auto set = fringeNodeSet.find(foreignCommunityIds[i]);
			if (set != fringeNodeSet.end()) {
			    for (auto node:*set) {
                        	if ( !(node.IsEqual(Ipv4Address("0.0.0.0"))) )
                        	{
                            	    PendingResponseEntry entry(requesterId, node,
                                        requestedContent, broadcastId, Ipv4Address("0.0.0.0"), requesterCommunityId, *set);
		                //ProcessPendingInterest(entry, interestEntry);
                                    if (!CheckIfPendingResponseExist(m_pending_interest_response,entry))
                                    {
                                        m_pending_interest_response->push_back(entry);
                                        NS_LOG_INFO("Save pending INTEREST response entry to m_pending_interest_response");
                                /*if (! (m_interestManager->Exist(interestEntry)) )
                                {
                                    m_interestManager->Insert(interestEntry);
                                }*/
                                    }
                                }
                            }
                            //propagate this to border node so that upon HandleHello, if border node directly
                            //encounters a foreign node, it will inject into that community.
                            //We do that because border node has high chance to encounter foreign nodes.
                        }
                        //else do nothing here. If I am already the bestBorderNot, then that's cool.
                        //Next time I encounter a foreign node, I will inject interest to foreign community.
                    }
                }
            }  
        }
    }
    //}
    /*else
    {
        // Need not to do anything here because:
        // InterestEntry exists meaning I already saw it before.
        // That means I already serve DATA packet for that interest before.
        // No need to serve it again.
    }*/
 
        
}  

bool
SocialNetwork::CheckIfPendingResponseExist(vector<PendingResponseEntry> *responseVec, PendingResponseEntry entry)
{
    for (uint32_t i=0;i<responseVec->size();++i)
    {
        if ( ( (responseVec->at(i)).requesterId).IsEqual(entry.requesterId) &&
             ( (responseVec->at(i)).destinationId).IsEqual(entry.destinationId) &&
             ( (responseVec->at(i)).contentRequested).IsEqual(entry.contentRequested) &&
             ( (responseVec->at(i)).broadcastId == entry.broadcastId) &&
             ( (responseVec->at(i)).foreignDestinationId).IsEqual(entry.foreignDestinationId) &&
             ( (responseVec->at(i)).requesterCommunityId == entry.requesterCommunityId) &&
             ( (responseVec->at(i)).bestBorderNodeId). IsEqual(entry.bestBorderNodeId) )
        {
            return true;
        }
    }
    return false;
}
 
void 
SocialNetwork::ProcessPendingDataResponse(PendingResponseEntry &entry, 
				      InterestEntry &interestEntry)
{
    if (!CheckIfPendingResponseExist(m_pendingDataResponse,entry))
    {
        m_pendingDataResponse->push_back(entry);
        NS_LOG_INFO("Save pending DATA response entry to m_pendingDataResponse");
        if (! (m_interestManager->Exist(interestEntry)) )
        {
            m_interestManager->Insert(interestEntry);
            global_count_interest++;
        }
    }
}

void
SocialNetwork::ProcessPendingInterest(PendingResponseEntry &entry,
				      InterestEntry & interestEntry)
{
    if (!CheckIfPendingResponseExist(m_pending_interest_response,entry))
    {
        //I am not relay to border node or content provider
        m_pending_interest_response->push_back(entry);
        NS_LOG_INFO("Save pending INTEREST response entry to m_pending_interest_response");
        if (! (m_interestManager->Exist(interestEntry)) )
        {
            m_interestManager->Insert(interestEntry);
            global_count_interest++;
        }
    }

}

void
SocialNetwork::ProcessPendingContent(PendingResponseEntry &entry,
				     InterestEntry &interestEntry)
{
    if (!CheckIfPendingResponseExist(m_pending_content_dest_node_response,entry))
    {
	m_pending_content_dest_node_response->push_back(entry);
	if (! (m_interestManager->Exist(interestEntry)) )
	{
	    m_interestManager->Insert(interestEntry);
	    global_count_interest++;
	}
    }
}   

}

