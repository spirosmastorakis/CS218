#include "ns3/log.h"
#include "ns3/pkt-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PktHeader");
NS_OBJECT_ENSURE_REGISTERED (PktHeader);

std::string
GetPacketTypeName(PacketType packetType)
{
	switch (packetType)
	{
	case HELLO:
		return "HELLO";
	case INTEREST:
		return "INTEREST";
	case DATA:
		return "DATA";
    case DIGEST:
        return "DIGEST";
	}
	return "";
}

PktHeader::PktHeader ()
{
}

PktHeader::~PktHeader ()
{
}

TypeId
PktHeader::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::PktHeader")
        .SetParent<Header> ()
        .AddConstructor<PktHeader> ()
        ;
        return tid;
}

TypeId
PktHeader::GetInstanceTypeId (void) const
{
    return GetTypeId ();
}

void
PktHeader::Serialize (Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteHtonU32 (m_source.Get ());
    i.WriteHtonU32 (m_destination.Get ());
    
    i.WriteHtonU32 ((uint32_t)m_packetType);
    i.WriteHtonU32 ((uint32_t)m_socialTieTable);
    i.WriteHtonU32 (m_socialTieTableSize);
    
    i.WriteHtonU32 ((uint32_t)m_higherCentralityList);
    i.WriteHtonU32 (m_higherCentralityListSize);
    
    i.WriteHtonU32 ((uint32_t)m_contentArray);
    i.WriteHtonU32 (m_contentArraySize);
   
    i.WriteHtonU32 (m_interestBroadcastId);
    i.WriteHtonU32 (m_requestedContent.Get());
    
    i.WriteHtonU32 (m_requesterId.Get ());
    i.WriteHtonU32 (m_contentProviderId.Get ());
	
	i.WriteHtonU32 (m_communityId);
	i.WriteHtonU32 (m_requesterCommunityId);
	
	i.WriteHtonU32 (m_foreignDestinationId.Get ());
        for (int k = 0; k< int(m_bestBorderNodeId.size ()); k++){
		i.WriteHtonU32 (m_bestBorderNodeId[k].Get ());
    	}
	i.WriteHtonU32 (m_borderNodeNumber);
}

uint32_t
PktHeader::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator  i = start;

    m_source.Set (i.ReadNtohU32 ());
    m_destination.Set (i.ReadNtohU32 ());
    
   	m_packetType = (PacketType)(i.ReadNtohU32());
   	m_socialTieTable = (uint32_t *)(i.ReadNtohU32 ());
   	m_socialTieTableSize = i.ReadNtohU32 ();
   	
   	m_higherCentralityList = (uint32_t *)(i.ReadNtohU32 ());
   	m_higherCentralityListSize = i.ReadNtohU32 ();
   	
   	m_contentArray = (uint32_t *)(i.ReadNtohU32 ());
   	m_contentArraySize = i.ReadNtohU32 ();
   	
   	m_interestBroadcastId = i.ReadNtohU32 ();
   	m_requestedContent.Set(i.ReadNtohU32 ());
   	
   	m_requesterId.Set (i.ReadNtohU32 ());
   	m_contentProviderId.Set (i.ReadNtohU32 ());
   	
	m_communityId = i.ReadNtohU32 ();
	m_requesterCommunityId = i.ReadNtohU32 ();
	
	m_foreignDestinationId.Set (i.ReadNtohU32 ());
	m_bestBorderNodeId.clear();
	int k = 0;
	Buffer::Iterator temp = i;
	temp.Next (4);
	while (!(temp.IsEnd ())) {
		m_bestBorderNodeId[k].Set (i.ReadNtohU32 ());
		k++;
		temp.Next (4);
	}
	
    m_borderNodeNumber = m_bestBorderNodeId.size();
    return GetSerializedSize ();
}

uint32_t
PktHeader::GetSerializedSize (void) const
{
    // Multiply by 17 for # of variables serialized
    return ((16+int(m_borderNodeNumber)) * (sizeof (uint32_t)));
}

void
PktHeader::Print (std::ostream &os) const
{
    os << "Src = " << m_source << "Dest = " << m_destination << std::endl; 
}

void
PktHeader::SetSource (Ipv4Address src)
{
    m_source = src;
}

void 
PktHeader::SetDestination (Ipv4Address dest)
{
    m_destination = dest;
}

Ipv4Address
PktHeader::GetSource (void) const
{
    return m_source;
}

Ipv4Address
PktHeader::GetDestination (void ) const
{
    return m_destination;
}

PacketType
PktHeader::GetPacketType() const
{
	return m_packetType;
}

void
PktHeader::SetPacketType(PacketType packetType)
{
	m_packetType = packetType;
}

uint32_t *
PktHeader::GetSocialTieTable() const
{
   	void setRequestedContent(uint32_t *requestedContent);
    return m_socialTieTable;
}

void
PktHeader::SetSocialTieTable(uint32_t *socialTieTable)
{
    m_socialTieTable = socialTieTable;
}

uint32_t
PktHeader::GetSocialTieTableSize() const
{
    return m_socialTieTableSize;
}

void
PktHeader::SetSocialTieTableSize(uint32_t socialTieTableSize)
{
    m_socialTieTableSize = socialTieTableSize;
}

uint32_t
PktHeader::GetHigherCentralityListSize() const
{
    return m_higherCentralityListSize;
}

void
PktHeader::SetHigherCentralityListSize(uint32_t higherCentralityListSize)
{
    m_higherCentralityListSize = higherCentralityListSize;
}
   	
uint32_t *
PktHeader::GetHigherCentralityList() const
{
    return m_higherCentralityList;
}

void
PktHeader::SetHigherCentralityList(uint32_t *higherCentralityList)
{
    m_higherCentralityList = higherCentralityList;
}

uint32_t
PktHeader::GetContentArraySize() const
{
    return m_contentArraySize;
}

void
PktHeader::SetContentArraySize(uint32_t contentArraySize)
{
    m_contentArraySize = contentArraySize;
}
   	
uint32_t *
PktHeader::GetContentArray() const
{
    return m_contentArray;
}

void
PktHeader::SetContentArray(uint32_t *contentArray)
{
    m_contentArray = contentArray;
}

//For interest packet
uint32_t
PktHeader::GetInterestBroadcastId() const
{
    return m_interestBroadcastId;
}

void
PktHeader::SetInterestBroadcastId(uint32_t interestBroadcastId)
{
    m_interestBroadcastId = interestBroadcastId;
}

Ipv4Address
PktHeader::GetRequestedContent() const
{
    return m_requestedContent;
}

void 
PktHeader::SetRequestedContent(Ipv4Address requestedContent)
{
    m_requestedContent = requestedContent;
}

Ipv4Address
PktHeader::GetRequesterId(void) const
{
    return m_requesterId;
}

void
PktHeader::SetRequesterId(Ipv4Address requesterId)
{
    m_requesterId = requesterId;
}

Ipv4Address
PktHeader::GetContentProviderId(void) const
{
    return m_contentProviderId;
}

uint32_t
PktHeader::GetCommunityId (void) const
{
    return m_communityId;
}

void
PktHeader::SetCommunityId (uint32_t commId)
{
    m_communityId = commId;
}

void
PktHeader::SetContentProviderId(Ipv4Address contentProviderId)
{
    m_contentProviderId = contentProviderId;
}

void
PktHeader::SetRequesterCommunityId (uint32_t requesterCommunityId)
{
    m_requesterCommunityId = requesterCommunityId;
}

uint32_t
PktHeader::GetRequesterCommunityId (void) const
{
    return m_requesterCommunityId;
}

void
PktHeader::SetForeignDestinationId (Ipv4Address foreignDestinationId)
{
    m_foreignDestinationId = foreignDestinationId;
}

Ipv4Address
PktHeader::GetForeignDestinationId (void) const
{
    return m_foreignDestinationId;
}

// Qiuhan: change
//probably needs to be changed. I think it is ok for now....
void
PktHeader::SetBestBorderNodeId(std::std<Ipv4Address> bestBorderNodeId)
{
  for(auto node: bestBorderNodeId)
  {
    m_bestBorderNodeId.push_back(node);
    m_borderNodeNumber++;
  }
}
/*
void
PktHeader::SetBestBorderNodeId(Ipv4Address bestBorderNodeId)
{
    m_bestBorderNodeId.push_back (bestBorderNodeId);
    m_borderNodeNumber++;
}*/


//needs to be changed
std::vector<Ipv4Address>
//Ipv4Address
PktHeader::GetBestBorderNodeId (void) const
{
    return m_bestBorderNodeId;
}


}
