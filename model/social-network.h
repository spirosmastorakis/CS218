/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef SOCIAL_NETWORK_H
#define SOCIAL_NETWORK_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"

#include "ns3/pkt-header.h"
#include "ns3/content-manager.h"
#include "ns3/relationship.h"
#include "ns3/interest-manager.h"
#include "ns3/community-manager.h"
#include "ns3/qhs.h"
#include <map>

using namespace std;

namespace ns3 {

struct PendingResponseEntry //each entry is uniquely identified by <requesterId, broadcastId>
{
    PendingResponseEntry() //This is rarely called
    {
        for (uint32_t i=0;i<100;++i)
        {
            this->lastRelayNode[i] = Ipv4Address("0.0.0.0"); //similar to NULL address
        }
        this->foreignDestinationId = Ipv4Address("0.0.0.0");
        this->bestBorderNodeId = Ipv4Address("0.0.0.0");
    }
    
    //Only "m_pending_content_dest_node_response" needs to have a valid destinationId
    //"m_pendingDataResponse" and "m_pending_interest_response" can set "255.255.255.255" for destinationId
    PendingResponseEntry(Ipv4Address requesterId, Ipv4Address destinationId, 
                         Ipv4Address contentRequested, uint32_t broadcastId,
                         Ipv4Address foreignDestinationId, uint32_t requesterCommunityId,
                         vector<Ipv4Address> &bestBorderNodeId)
    {
        this->requesterId = requesterId;
        this->destinationId = destinationId; //final destination for this pending entry
            //For example: final destination for DATA packet
            //final destination for an INTEREST packet which is the known content provider ID.
        
        this->contentRequested = contentRequested;
        this->broadcastId = broadcastId;
        for (uint32_t i=0;i<100;++i)
        {
            this->lastRelayNode[i] = Ipv4Address("0.0.0.0"); //similar to NULL address
        }
        this->foreignDestinationId = foreignDestinationId; //if this value is "0.0.0.0", it sifnifies that there is no
                                                        //routing to border node. Otherwise, we are routing to border node.
        this->requesterCommunityId = requesterCommunityId;
        this->bestBorderNodeId = bestBorderNodeId;
        
    }
    Ipv4Address requesterId; //Relay node needs this information because requesterId
                             //will be the ultimate destination. The immediate destination
                             //is the relay node the packet is unicasted to.
    Ipv4Address destinationId; //unicast INTEREST to node with higher social level
                               //unitcast INTEREST to node with higher social tie to node that has content
                               //unicast DATA to node the requester
    Ipv4Address lastRelayNode[100]; //up to 99 communities
    Ipv4Address contentDestinationNode; //used when a node knows which node has a content
                                        //then it needs to unicasts Interest using social tie routing to contentDestinationNode
    Ipv4Address contentRequested; //for INTEREST only
    uint32_t broadcastId; //Don't need this when sending DATA packet.
    Ipv4Address foreignDestinationId;
    uint32_t requesterCommunityId;
    vector<Ipv4Address> bestBorderNodeId;
};

class Socket;
class Packet;

/**
 * \ingroup udpecho
 * \brief A Udp Echo client
 *
 * Every packet sent should be returned by the server and received here.
 */
class SocialNetwork : public Application 
{
public:

    static uint32_t global_count_hello;
    static uint32_t global_count_interest;
    static uint32_t global_count_data;
    static TypeId GetTypeId (void);

      SocialNetwork ();

    virtual ~SocialNetwork ();

    /**
     * Set the data size of the packet (the number of bytes that are sent as data
     * to the server).  The contents of the data are set to unspecified (don't
     * care) by this call.
     *
     * \warning If you have set the fill data for the echo client using one of the
     * SetFill calls, this will undo those effects.
     *
     * \param dataSize The size of the echo data you want to sent.
     */
    void SetDataSize (uint32_t dataSize);

    /**
     * Get the number of data bytes that will be sent to the server.
     *
     * \warning The number of bytes may be modified by calling any one of the 
     * SetFill methods.  If you have called SetFill, then the number of 
     * data bytes will correspond to the size of an initialized data buffer.
     * If you have not called a SetFill method, the number of data bytes will
     * correspond to the number of don't care bytes that will be sent.
     *
     * \returns The number of data bytes.
     */
    uint32_t GetDataSize (void) const;

    /**
     * Set the data fill of the packet (what is sent as data to the server) to 
     * the zero-terminated contents of the fill string string.
     *
     * \warning The size of resulting echo packets will be automatically adjusted
     * to reflect the size of the fill string -- this means that the PacketSize
     * attribute may be changed as a result of this call.
     *
     * \param fill The string to use as the actual echo data bytes.
     */
    void SetFill (std::string fill);

    /**
     * Set the data fill of the packet (what is sent as data to the server) to 
     * the repeated contents of the fill byte.  i.e., the fill byte will be 
     * used to initialize the contents of the data packet.
     * 
     * \warning The size of resulting echo packets will be automatically adjusted
     * to reflect the dataSize parameter -- this means that the PacketSize
     * attribute may be changed as a result of this call.
     *
     * \param fill The byte to be repeated in constructing the packet data..
     * \param dataSize The desired size of the resulting echo packet data.
     */
    void SetFill (uint8_t fill, uint32_t dataSize);

    /**
     * Set the data fill of the packet (what is sent as data to the server) to
     * the contents of the fill buffer, repeated as many times as is required.
     *
     * Initializing the packet to the contents of a provided single buffer is 
     * accomplished by setting the fillSize set to your desired dataSize
     * (and providing an appropriate buffer).
     *
     * \warning The size of resulting echo packets will be automatically adjusted
     * to reflect the dataSize parameter -- this means that the PacketSize
     * attribute of the ApplicatiotentDestin may be changed as a result of this call.
     *
     * \param fill The fill pattern to use when constructing packets.
     * \param fillSize The number of bytes in the provided fill pattern.
     * \param dataSize The desired size of the final echo data.
     */
    void SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize);

    // Avoid using the helper to setup an app
    void Setup (uint16_t port);
    // Set content requestor
    void RequestContent (Ipv4Address content);

    uint32_t m_communityId;
protected:
    virtual void DoDispose (void);

private:

    virtual void StartApplication (void);
    virtual void StopApplication (void);

    void ScheduleTransmit (Time dt, PacketType packetType);
    void Send (PacketType packetType);

    void HandleRead (Ptr<Socket> socket);

    uint32_t m_count;
    Time m_interval;
    uint32_t m_size;

    uint32_t m_dataSize;
    uint8_t *m_data;

    uint32_t m_sent;
    Ptr<Socket> m_socket;
    uint16_t m_peerPort;
    EventId m_sendEvent;
    /// Callbacks for tracing thce packet Tx events
    TracedCallback<Ptr<const Packet> > m_txTrace;
    
    // TUAN
    Relationship *m_relationship;
    Ipv4Address m_initialOwnedContent;
    Ipv4Address m_initialRequestedContent;
    ContentManager *m_contentManager;
    InterestManager *m_interestManager;
    InterestManager *m_seenDataManager; //keep track of which data packets seen so far
                                    //We use the same InterestManager since we want essentially
                                   //the same format for both
    CommunityManager *m_foreignCommunityManager; //keep track of foreign communities
    uint32_t m_interestBroadcastId;
    
    vector<PendingResponseEntry> *m_pendingDataResponse;
    vector<PendingResponseEntry> *m_pending_content_dest_node_response;
    vector<PendingResponseEntry> *m_pending_interest_response;
    bool m_firstSuccess;
    bool m_meetForeign;
    
    //leave index 0 blank.
    //last_foreign_encounter_node[2] refers to last foreign encounter node in community 2
    Ipv4Address *m_last_foreign_encounter_node;
    
    // Spyros: Map of <community id, vector<Ipv4Address> >
    std::map<uint32_t, std::vector<Ipv4Address> > fringeNodeSet;
    
    QHS qhs;

    void ScheduleTransmitHelloPackets (int numberOfHelloEvents);
    Ipv4Address GetNodeAddress(void);
    void HandleData(PktHeader *header);
    void HandleHello(PktHeader *header);
    void HandleInterest(PktHeader *header);
    void HandleDigest(PktHeader *header);
    PktHeader *CreateDataPacketHeader(Ipv4Address destinationId, Ipv4Address requesterId,
                    Ipv4Address contentRequested, uint32_t requesterCommunityId,
                    Ipv4Address foreignDestinationId, vector<Ipv4Address> &bestBorderNode, uint32_t broadcastId);
    PktHeader *CreateHelloPacketHeader();
    PktHeader *CreateInterestPacketHeader(Ipv4Address requesterId, Ipv4Address destinationId,
                Ipv4Address contentProviderId, Ipv4Address contentRequested, uint32_t broadcastId, 
                uint32_t requesterCommunityId, Ipv4Address foreignDestinationId,
                vector<Ipv4Address> &bestBorderNodeId);
    PktHeader *CreateDigestPacketHeader(Ipv4Address destinationId);
    void HandlePendingDataResponse(PktHeader *header);
    void HandlePendingInterestResponse(PktHeader *header);
    void HandlePendingContentDestNodeResponse(PktHeader *header);
    void DecideWhetherToSendContentNameDigest(PktHeader *header);

    void ProcessPendingDataResponse(PendingResponseEntry &entry, 
				    InterestEntry &interestEntry);
    void ProcessPendingInterest(PendingResponseEntry &entry,
				InterestEntry &interestEntry);
    void ProcessPendingContent(PendingResponseEntry &entry,
			       InterestEntry &interestEntry);
    


    //Print content line by line
    void PrintAllContent(ContentOwnerEntry *array, uint32_t size);
    
    void SendPacket(PktHeader header);
    
    //used to avoid adding duplicate entries into the pending response vector
    bool CheckIfPendingResponseExist(vector<PendingResponseEntry> *responseVec, PendingResponseEntry entry);
    
    
};

}

#endif /* SOCIAL_NETWORK_H */

