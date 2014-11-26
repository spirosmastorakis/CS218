
#ifndef PKT_HEADER_H
#define PKT_HEADER_H

#include "ns3/header.h"
#include "ns3/address-utils.h"
#include "ns3/ipv4-address.h"

using namespace std;

namespace ns3
{

//tuan
enum PacketType { HELLO, INTEREST, DATA, DIGEST };

string GetPacketTypeName(PacketType packetType);

class PktHeader : public Header
{
public:
    PktHeader ();
    virtual ~PktHeader ();
    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId (void) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);
    virtual uint32_t GetSerializedSize (void) const;
    virtual void Print (std::ostream &os) const;
    void SetSource (Ipv4Address src);
    void SetDestination (Ipv4Address dest);
    Ipv4Address GetSource (void) const;
    Ipv4Address GetDestination (void ) const;
    
    PacketType GetPacketType() const;
   	void SetPacketType(PacketType packetType);
   	uint32_t *GetSocialTieTable() const;
   	void SetSocialTieTable(uint32_t *socialTieTable);
   	uint32_t GetSocialTieTableSize() const;
   	void SetSocialTieTableSize(uint32_t socialTieTableSize);
   	
   	uint32_t GetHigherCentralityListSize() const;
   	void SetHigherCentralityListSize(uint32_t higherCentralityListSize);
   	
   	uint32_t *GetHigherCentralityList() const;
   	void SetHigherCentralityList(uint32_t *higherCentralityList);
   	
   	uint32_t GetContentArraySize() const;
   	void SetContentArraySize(uint32_t contentArraySize);
   	
   	uint32_t *GetContentArray() const;
   	void SetContentArray(uint32_t *contentArray);
   	
   	//For interest packet
   	uint32_t GetInterestBroadcastId() const;
   	void SetInterestBroadcastId(uint32_t interestBroadcastId);

   	Ipv4Address GetRequestedContent() const;
   	void SetRequestedContent(Ipv4Address requestedContent);
   	
   	Ipv4Address GetRequesterId(void) const;
   	void SetRequesterId(Ipv4Address requesterId);
   	
   	Ipv4Address GetContentProviderId(void) const;
   	void SetContentProviderId(Ipv4Address contentProviderId);
    
    void SetCommunityId (uint32_t commId);
    uint32_t GetCommunityId (void) const;
    
    void SetRequesterCommunityId (uint32_t requesterCommunityId);
    uint32_t GetRequesterCommunityId (void) const;
    
    void SetForeignDestinationId (Ipv4Address foreignDestinationId);
    Ipv4Address GetForeignDestinationId (void) const;
    
    void SetBorderNode (uint32_t *borderNode);
    uint32_t *GetBorderNode (void) const;

    void SetBorderNodeSize(uint32_t borderNodeSize);
    uint32_t GetBorderNodeSize(void) const;

private:
    // These 2 fields uniquely identify an Interest packet
    Ipv4Address m_source;
    uint32_t m_interestBroadcastId;
    Ipv4Address m_requestedContent; //a string representing the name of the requested content
    uint32_t m_communityId;
    Ipv4Address m_destination;
    uint16_t m_headerSize;
    uint32_t *m_socialTieTable;
    uint32_t m_socialTieTableSize;
    uint32_t m_higherCentralityListSize;
    uint32_t *m_higherCentralityList;
    uint32_t m_contentArraySize;
    uint32_t *m_contentArray;
    Ipv4Address m_requesterId;
    Ipv4Address m_contentProviderId;
    uint32_t m_requesterCommunityId;
    Ipv4Address m_foreignDestinationId;
    uint32_t *m_borderNode;
    //std::vector<Ipv4Address> m_bestBorderNodeId;
    uint32_t m_borderNodeSize;
    
    // Add TTL and other stuff
    
    PacketType m_packetType;

};

}	// namespace ns3

#endif /* PKT_HEADER_H */
