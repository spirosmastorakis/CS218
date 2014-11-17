
// TUAN: WRITE TO LOG FILE
//./waf --run scratch/topology6 >log.out 2>&1
//./waf --run "scratch/mobTop5 --nodeNum=50 --traceFile=scratch/sc2.tcl" > log.out 2>&1



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
    m_interestBroadcastId = 0;
    m_pendingDataResponse = new vector<PendingResponseEntry>;
    m_pending_content_dest_node_response = new vector<PendingResponseEntry>;
    m_pending_interest_response = new vector<PendingResponseEntry>;
}

SocialNetwork::~SocialNetwork()
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;

    delete [] m_data;
    delete m_contentManager;
    delete m_interestManager;
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
    ContentOwnerEntry content(thisNodeAddress, m_initialOwnedContent);
    m_contentManager->Insert(content);
    
    //insert the initial requested content into m_pending_interest_response
    if ( !( m_initialRequestedContent.IsEqual(Ipv4Address("0.0.0.0"))) )
    {
        NS_LOG_INFO(""<<thisNodeAddress<<" requests content: "<<m_initialRequestedContent);
        PendingResponseEntry entry(thisNodeAddress,
                Ipv4Address("255.255.255.255"), m_initialRequestedContent, m_interestBroadcastId);
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
    
    ScheduleTransmitHelloPackets(40);
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
    const double TIME_INTERVAL = 0.1; //100ms
    
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
        //NS_LOG_INFO("Requested Content: "<<m_initialRequestedContent);
        //header = CreateInterestPacketHeader(GetNodeAddress(), Ipv4Address("255.255.255.255"), &m_initialRequestedContent, m_interestBroadcastId); //have it for now, remove later
        //m_interestBroadcastId++; //remove this also
            //In HandleHello, when requesterId matches GetNodeAddress(), we increment m_interestBroadcastId
            //before sending the Interest packet for the first time.
        //we should never schedule to transmit an INTEREST, or DATA packet.
        //Only HELLO packet needs to be scheduled to transmit.
        
    }
    
    //Add header to the packet
    p->AddHeader(*header);
    
    // call to the trace sinks before the packet is actually sent,
    // so that tags added to the packet can be sent as well
    m_txTrace (p);
    m_socket->Send (p);

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
SocialNetwork::CreateDataPacketHeader(Ipv4Address destinationId, Ipv4Address requesterId, Ipv4Address contentRequested)
{
    PktHeader *header = new PktHeader();
    header->SetSource(GetNodeAddress());
    header->SetDestination(destinationId);
    header->SetRequesterId(requesterId);
    header->SetRequestedContent(contentRequested); //actually, it should send the content data
    header->SetCommunityId(m_communityId);
        //we don't really need to include the contentRequested value in DATA packet header.
    header->SetPacketType(DATA);
    
    return header;
}

PktHeader *
SocialNetwork::CreateInterestPacketHeader(Ipv4Address requesterId, Ipv4Address destinationId,
            Ipv4Address contentProviderId, Ipv4Address contentRequested, uint32_t broadcastId)
{
    PktHeader *header = new PktHeader();
    header->SetSource(GetNodeAddress());
    header->SetDestination(destinationId);
    header->SetContentProviderId(contentProviderId);
    header->SetRequestedContent(contentRequested);
    
    //The node that has the content, when receiving this packet, it will check
    //the requesterId and broadcastId and broadcastId of the packet, if it already exists
    //in its interestManager, it will discard the packet even though it has DATA matches
    //that interest.
    header->SetRequesterId(requesterId); 
    header->SetInterestBroadcastId(broadcastId);
    
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
    Ipv4Address currentNode = GetNodeAddress();
    Ipv4Address requestedContent = header->GetRequestedContent();
    
    /*
        Algorithm:
        If DATA packet is destined to me:
            SUCCESS
        Else:
            If this DATA packet not in my m_pendingDataResponse:
                Turn into DATA social tie routing by by saving
                (requesterId, content_requested, broadcast_id) into m_pendingDataResponse
    */  
    if ( currentNode.IsEqual(header->GetRequesterId()) )
    {
        NS_LOG_INFO ("SUCCESS!! SUCCESS!! SUCCESS!! SUCCESS!! SUCCESS!!");
        NS_LOG_INFO ("Requester node "<<currentNode<<" receives requested content "<<requestedContent <<" from node: "<<header->GetSource());
    }
    else
    {
        for (vector<PendingResponseEntry>::iterator it = m_pendingDataResponse->begin();
             it != m_pendingDataResponse->end(); ++it)
        {
            //Note: Each PendingResponseEntry is uniquely identified by <requesterId, broadcastId>
            if ( (header->GetRequesterId()).IsEqual(it->requesterId) &&
                 header->GetInterestBroadcastId() == it->broadcastId )
            {
                return; //entry already exists.
            }
        }
        
        NS_LOG_INFO("Inside HandleData: save pending DATA response entry to m_pendingDataResponse");
        PendingResponseEntry entry(header->GetRequesterId(), header->GetRequesterId(),
                                   requestedContent, header->GetInterestBroadcastId());
        m_pendingDataResponse->push_back(entry);
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
        PrintAllContent(contentArray, contentArraySize);
        NS_LOG_INFO("This node content before merge: ");
        PrintAllContent(m_contentManager->GetContentArray(), m_contentManager->GetContentArraySize());
        
        m_contentManager->Merge(contentArray, contentArraySize);
                    
        NS_LOG_INFO("This node content after merge: ");
        PrintAllContent(m_contentManager->GetContentArray(), m_contentManager->GetContentArraySize());
    }
}

void
SocialNetwork::HandleHello(PktHeader *header)
{
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
    
    DecideWhetherToSendContentNameDigest(header);
    
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
SocialNetwork::DecideWhetherToSendContentNameDigest(PktHeader *header)
{
    Ipv4Address currentNode = GetNodeAddress();
    Ipv4Address encounterNode = header->GetSource();
    
    Ipv4Address higherCentralityNode = m_relationship->GetHigherCentralityNode(currentNode, encounterNode);
    if (higherCentralityNode.IsEqual(encounterNode))
    {
        //Unicast content name digest packet to encounter node
        NS_LOG_INFO(""<<currentNode<<" sends content name digest to node "<<encounterNode);
        Ptr<Packet> p = Create<Packet> (m_size);
        PktHeader *header = CreateDigestPacketHeader(encounterNode);
        p->AddHeader(*header);
        m_txTrace (p);
        m_socket->Send (p);
    }
}

void
SocialNetwork::HandlePendingContentDestNodeResponse(PktHeader *header)
{
    Ipv4Address currentNode = GetNodeAddress();
    Ipv4Address encounterNode = header->GetSource();
    //If encounter node (sourceAddr) matches content provider ID in m_pending_content_dest_node_response entry
    //then unicast interest immediately. Otherwise, continue following social-tie routing towards content provider.
    
    /*
    Algorithm:
    For each response in m_pending_content_dest_node_response:
        If encounter is the target content provider node:
            Unicast INTEREST packet to encounter
        Else:
            If encounter has higher social tie toward target content provider than me:
                If last_relay_encounter == NULL:
                    Unicast INTEREST packet to encounter
                    Update last_relay_encounter to encounter
                Else:
                    If encounter has higher social tie toward content provider than last_relay_encounter:
                        Unicast INTEREST packet to encounter
                        Update last_relay_encounter to encounter
    */
    
    NS_LOG_INFO("m_pending_content_dest_node_response size: "<<m_pending_content_dest_node_response->size());
    
    for (vector<PendingResponseEntry>::iterator it = m_pending_content_dest_node_response->begin();
         it != m_pending_content_dest_node_response->end(); ++it)
    {
        if ( (it->destinationId).IsEqual(encounterNode) ) //encounter node is the content provider
        {
            //Unicast INTEREST packet
            Ptr<Packet> p = Create<Packet> (m_size);
            PktHeader *header = CreateInterestPacketHeader(it->requesterId, encounterNode,
                                it->destinationId, it->contentRequested, it->broadcastId);
            p->AddHeader(*header);
            m_txTrace (p);
            m_socket->Send (p);
            
            NS_LOG_INFO("Send INTEREST packet with content ("<<it->contentRequested<<") to "<<encounterNode);
            NS_LOG_INFO("The target content provider is: "<<it->destinationId);
        }
        else //encounter node is not the content provider, use social tie routing to propagate INTEREST packet
        {
            Ipv4Address higherSocialTieNode =
                    m_relationship->GetHigherSocialTie(currentNode, encounterNode, it->destinationId);
            if (higherSocialTieNode.IsEqual(encounterNode))
            {
                if ( (it->lastRelayNode).IsEqual(Ipv4Address("0.0.0.0")) ) //test if lastRelayNode is NULL
                {
                    it->lastRelayNode = encounterNode;
                    
                    //Unicast INTEREST packet
                    Ptr<Packet> p = Create<Packet> (m_size);
                    PktHeader *header = CreateInterestPacketHeader(it->requesterId, encounterNode,
                                it->destinationId, it->contentRequested, it->broadcastId);
                    p->AddHeader(*header);
                    m_txTrace (p);
                    m_socket->Send (p);
            
                    NS_LOG_INFO("Send INTEREST packet with content ("<<it->contentRequested<<") to "<<encounterNode);
                    NS_LOG_INFO("The target content provider is: "<<it->destinationId);
                }
                else
                {
                    Ipv4Address higherSocialTieNode2 =
                            m_relationship->GetHigherSocialTie(encounterNode, it->lastRelayNode, it->destinationId);
                    if ( higherSocialTieNode2.IsEqual(encounterNode) )
                    {
                        it->lastRelayNode = encounterNode;
                        
                        //Unicast INTEREST packet
                        Ptr<Packet> p = Create<Packet> (m_size);
                        PktHeader *header = CreateInterestPacketHeader(it->requesterId, encounterNode,
                                it->destinationId, it->contentRequested, it->broadcastId);
                        p->AddHeader(*header);
                        m_txTrace (p);
                        m_socket->Send (p);
                
                        NS_LOG_INFO("Send INTEREST packet with content ("<<it->contentRequested<<") to "<<encounterNode);
                        NS_LOG_INFO("The target content provider is: "<<it->destinationId);
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

    // Take care of m_pending_interest_response
    // These are pending interests that do not know which node owns the requested content.
    /*
    Algorithm:
    For each response in m_pending_interest_response:
        If encounter has higher social level than me:
            If last_data_relay_encounter == NULL:
                Unicast INTEREST packet to encounter
                Update last_relay_encounter to encounter
            Else:
                If encounter has higher social level than last_relay_encounter:
                    Unicast INTEREST packet to encounter
                    Update last_relay_encounter to encounter
    */
    
    NS_LOG_INFO("m_pending_interest_response size: "<<m_pending_interest_response->size());
    for (vector<PendingResponseEntry>::iterator it = m_pending_interest_response->begin();
         it != m_pending_interest_response->end(); ++it)
    {
        Ipv4Address higherSocialLevelNode = m_relationship->GetHigherSocialLevel(currentNode, encounterNode);
        if ( higherSocialLevelNode.IsEqual(encounterNode) )
        {
            if ( (it->lastRelayNode).IsEqual(Ipv4Address("0.0.0.0")) ) //test if lastRelayNode is NULL
            {
                it->lastRelayNode = encounterNode;
                
                //Unicast INTEREST packet
                Ptr<Packet> p = Create<Packet> (m_size);
                PktHeader *header = CreateInterestPacketHeader(it->requesterId, encounterNode,
                                                Ipv4Address("0.0.0.0"), it->contentRequested, it->broadcastId);
                p->AddHeader(*header);
                m_txTrace (p);
                m_socket->Send (p);
        
                NS_LOG_INFO("Send INTEREST packet with content ("<<it->contentRequested<<") to "<<encounterNode);
            }
            else
            {
                Ipv4Address higherSocialLevelNode2 = m_relationship->GetHigherSocialLevel(encounterNode, it->lastRelayNode);
                if ( higherSocialLevelNode2.IsEqual(encounterNode) )
                {
                    it->lastRelayNode = encounterNode;
                
                    //Unicast INTEREST packet
                    Ptr<Packet> p = Create<Packet> (m_size);
                    PktHeader *header = CreateInterestPacketHeader(it->requesterId, encounterNode,
                                                    Ipv4Address("0.0.0.0"), it->contentRequested, it->broadcastId);
                    p->AddHeader(*header);
                    m_txTrace (p);
                    m_socket->Send (p);
            
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
    
    //Take care of m_pendingDataResponse.
    /*
    Algorithm:
    For each response in m_pendingDataResponse:
        If the encounter node is the destination node the DATA packet should be destined to.
            Unicast DATA packet to the encounter
        Else:
            If encounter node has higher social tie toward requester X than me:
                If last_relay_encounter == NULL:
                    Unicast DATA packet to encounter node
                    Update last_relay_encounter to encounter
                Else:
                    If encounter node has higher social tie toward requester X than last_relay_encounter:
                        Unicast DATA packet to encounter node
                        Update last_relay_encounter to encounter
    */
    
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
            //remove_index.push_back(i);
            //Send DATA packet
            Ptr<Packet> p = Create<Packet> (m_size);
            PktHeader *header = CreateDataPacketHeader(encounterNode, it->requesterId, it->contentRequested);
            p->AddHeader(*header);
            m_txTrace (p);
            m_socket->Send (p);
            
            NS_LOG_INFO("Send DATA packet with content ("<<it->contentRequested<<") to "<<encounterNode);
        }
        else //encounter node is not the requester node, use social tie routing to propagate DATA packet
        {
            Ipv4Address higherSocialTieNode =
                    m_relationship->GetHigherSocialTie(currentNodeAddress, encounterNode, it->requesterId);
            NS_LOG_INFO("current node address: "<<currentNodeAddress);
            NS_LOG_INFO("encounter node: "<<encounterNode);
            NS_LOG_INFO("requester id: "<<it->requesterId);
            NS_LOG_INFO("higher social tie node: "<<higherSocialTieNode);
            m_relationship->PrintSocialTable();

            if (higherSocialTieNode.IsEqual(encounterNode))
            {
                if ( (it->lastRelayNode).IsEqual(Ipv4Address("0.0.0.0")) ) //test if lastRelayNode is NULL
                {
                    //remove_index.push_back(i);
                    it->lastRelayNode = encounterNode;
                    
                    //Send DATA packet
                    Ptr<Packet> p = Create<Packet> (m_size);
                    PktHeader *header = CreateDataPacketHeader(encounterNode, it->requesterId, it->contentRequested);
                    p->AddHeader(*header);
                    m_txTrace (p);
                    m_socket->Send (p);
            
                    NS_LOG_INFO("Send DATA packet with content ("<<it->contentRequested<<") to "<<encounterNode);
                }
                else
                {
                    Ipv4Address higherSocialTieNode2 =
                            m_relationship->GetHigherSocialTie(encounterNode, it->lastRelayNode, it->requesterId);
                    if ( higherSocialTieNode2.IsEqual(encounterNode) )
                    {
                        //remove_index.push_back(i);
                        it->lastRelayNode = encounterNode;
                        
                        //Send DATA packet
                        Ptr<Packet> p = Create<Packet> (m_size);
                        PktHeader *header = CreateDataPacketHeader(encounterNode, it->requesterId, it->contentRequested);
                        p->AddHeader(*header);
                        m_txTrace (p);
                        m_socket->Send (p);
                
                        NS_LOG_INFO("Send DATA packet with content ("<<it->contentRequested<<") to "<<encounterNode);
                    }
                }
            }
        }
        
        //++i;
    } //end of for loop
    
    /*for (vector<uint32_t>::iterator it = remove_index.begin(); it != remove_index.end(); ++it)
    {
        //NS_LOG_INFO("TUANTUAN size before: "<<m_pendingDataResponse->size());
        m_pendingDataResponse->erase(m_pendingDataResponse->begin() + *it);
        //NS_LOG_INFO("TUANTUAN size after: "<<m_pendingDataResponse->size());
    }*/
}

void
SocialNetwork::PrintAllContent(ContentOwnerEntry *array, uint32_t size)
{
    for (uint32_t i=0;i<size;++i)
    {
        NS_LOG_INFO( array[i].contentName );
    }
}

void
SocialNetwork::HandleInterest(PktHeader *header)
{
    Ipv4Address currentNodeAddress = GetNodeAddress();
    Ipv4Address encounterNode = header->GetSource();
    
    // 1) If receiving the same interest, ignore the interest!
    //    Each interest is uniquely identified by <original requester ID, broadcast_id>
    //    These 2 fields are found in packet header.
    //    We don't use content name as unique identifier since it might be the case that
    //    after receiving the DATA, the original requester might want to request the same content again.
    
    Ipv4Address requesterId = header->GetRequesterId();
    Ipv4Address contentProviderId = header->GetContentProviderId();
    uint32_t broadcastId = header->GetInterestBroadcastId();
    Ipv4Address requestedContent = header->GetRequestedContent();
    NS_LOG_INFO("Interest packet - requesterId: "<<requesterId);
    NS_LOG_INFO("Interest packet - contentProviderId: "<<requesterId);
    NS_LOG_INFO("Interest packet - broadcastId: "<<broadcastId);
    NS_LOG_INFO("Interest packet - requestedContent: "<<requestedContent);
    //NS_LOG_INFO("This node has: "<<GetAllContentInOneString(m_contentManager->GetContentArray(),
    //                                                        m_contentManager->GetContentArraySize()));
    InterestEntry interestEntry(requesterId, broadcastId, requestedContent);
        //dynamically allocated memory to avoid local value being destroyed.
    
    /*
    Algorithm:
        If not see this Interest before:
            Insert the interest into my InterestManager
            If I have the content:
                If encounter node is the requester:
                    send DATA packet to encounter node
                Else:
                    Turn into DATA social-tie routing by saving
                    (requesterId, content_requested, broadcast_id) into m_pendingDataResponse
            Else:
                If packet header tells me some node has the content (GetContentProviderId returns a valid address): 
                    Turn into INTEREST social-tie routing by saving
                    (requesterId, GetContentProviderId, broadcastId, contentRequested) into m_pending_content_dest_node_response
                Else If my table tells me some node has the content:
                    Turn into INTEREST social-tie routing by saving
                    (requesterId, contentOwnerId, broadcastId, contentRequested) into m_pending_content_dest_node_response
                    //NOTE: in HandleHello, a node will check its m_pending_content_dest_node_response
                    //For each pending X there, it will check if encouter social tie toward contentDestinationNode is higher
                    //then itself ...
                Else:
                    Turn into INTEREST social-level routing by saving
                    (requesterId, broadcastId, contentRequested) into m_pending_interest_response
    */
    if (! (m_interestManager->Exist(interestEntry)) )
    {
        //first time sees this Interest packet
        m_interestManager->Insert(interestEntry);
        
        //Check my local content table. If there is a match, then turn into social-tie routing
        Ipv4Address contentOwner = m_contentManager->GetOwnerOfContent(requestedContent);
        NS_LOG_INFO("Owner of content "<< requestedContent <<" is: "<<contentOwner);
        if (contentOwner.IsEqual(currentNodeAddress)) //I have the content
        {
            if (encounterNode.IsEqual(requesterId))
            {
                //Send DATA packet
                Ptr<Packet> p = Create<Packet> (m_size);
                PktHeader *header = CreateDataPacketHeader(encounterNode, requesterId, requestedContent);
                p->AddHeader(*header);
                m_txTrace (p);
                m_socket->Send (p);
        
                NS_LOG_INFO("Send DATA packet with content ("<< requestedContent <<") to "<<encounterNode);
            }
            else //Turn into DATA social-tie routing
            {
                NS_LOG_INFO("Inside HandleInterest: Save pending DATA response entry to m_pendingDataResponse");
                PendingResponseEntry entry(requesterId, requesterId, requestedContent, broadcastId);
                m_pendingDataResponse->push_back(entry);
                //will send DATA packet in HandleHello! 
                //Upon receiving Hello message, it checks if there is any pending DATA that needs to be sent.
                //If there is pending data, check if encounter node has higher social tie toward the requester
                //than the current node, also higher social tie than last encounter node. If yes => send DATA packet.
            }

        }
        else //I do not have the content
        {
            if ( !(contentProviderId.IsEqual(Ipv4Address("0.0.0.0"))) )
                //I am a relay node for an interest packet towards a known content provider
            {
                PendingResponseEntry entry(requesterId, contentProviderId, requestedContent, broadcastId);
                m_pending_content_dest_node_response->push_back(entry);
            }
            else if ( !(contentOwner.IsEqual(Ipv4Address("0.0.0.0"))) ) //My content table tells me which node has a content
            {
                PendingResponseEntry entry(requesterId, contentOwner, requestedContent, broadcastId);
                m_pending_content_dest_node_response->push_back(entry);
            }
            else //I don't know which node has the content
            {
                PendingResponseEntry entry(requesterId, Ipv4Address("255.255.255.255"), requestedContent, broadcastId);
                m_pending_interest_response->push_back(entry);
            }
        }
        
    }
    else
    {
        // Need not to do anything here because:
        // InterestEntry exists meaning I already saw it before.
        // That means I already serve DATA packet for that interest before.
        // No need to serve it again.
    }
    
}    
    

}

