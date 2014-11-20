#include "ns3/qhs.h"

namespace ns3 {

typedef std::pair<Ipv4Address, Ipv4Address> addrPair;

QHS::QHS () :
  m_localCommunity (0)
{
}

QHS::QHS (uint32_t localCommunity)
{
  m_localCommunity = localCommunity;
}

uint32_t
QHS::GetLocalCommunity ()
{
  return m_localCommunity;
}

void
QHS::SetLocalCommunity (uint32_t localCommunity) 
{
  m_localCommunity = localCommunity;
}

QHS::~QHS ()
{
}
  
uint32_t*
QHS::UpdateSocialTable (uint32_t *originalTable, uint32_t originalSize,
			uint32_t *newTable, uint32_t newSize,
			Ipv4Address ID)
{
  std::map<addrPair, SocialTableEntry> map;
  SocialTableEntry *socialTableEntry = (SocialTableEntry *)(*newTable);
  // push entry that may need to updated into a hash map for later reference
  for (int i = 0; i < int (newSize); i++) {
    if ((socialTableEntry[i].peer1.ID == ID && socialTableEntry[i].peer2.commID != m_localCommunity) || 
	(socialTableEntry[i].peer2.ID == ID && socialTableEntry[i].peer1.commID != m_localCommunity)) {
      map[std::make_pair(socialTableEntry[i].peer1.ID,socialTableEntry[i].peer2.ID)] =
        socialTableEntry[i];
    }
  }

  SocialTableEntry *originalEntry = (SocialTableEntry *)(*originalTable);
  
  for (int i = 0; i < int (originalSize); i++) { 
    if ((originalEntry[i].peer1.ID == ID && originalEntry[i].peer2.commID != m_localCommunity) || 
	(originalEntry[i].peer2.ID == ID && originalEntry[i].peer1.commID != m_localCommunity)) {
      auto it = map.find(std::make_pair(originalEntry[i].peer1.ID, originalEntry[i].peer2.ID));
      if (it != map.end()) {
        if (it->second.timestamp > originalEntry[i].timestamp) {
          originalEntry[i].socialTieValue = it->second.socialTieValue; 
        }
      }	
    }
  }
  return originalTable;
}

std::vector <Ipv4Address>
QHS::UpdateFringeNodeSet (uint32_t *socialTieTable, uint32_t socialTieTableSize)
{
  FringeNodeSet.clear ();
  double sum = 0;
  int number = 0;
  double max = 0;
  SocialTableEntry *socialTableEntry = (SocialTableEntry *) (*socialTieTable);
  for (int i = 0 ; i < int (socialTieTableSize); i++) {
    if ((socialTableEntry[i].peer1.commID == m_localCommunity) and (socialTableEntry[i].peer2.commID == m_localCommunity)) 
      continue;
    else {
      sum += socialTableEntry[i].socialTieValue;
      number++; 
      if (max < socialTableEntry[i].socialTieValue)
        max = socialTableEntry[i].socialTieValue;
    }
  }
  double avg = sum / number;
  float b = 0.5;
  double equation = avg + b * (max - avg);
  for (int i = 0 ; i < int (socialTieTableSize); i++) {
      if ((socialTableEntry[i].peer1.commID == m_localCommunity) and (socialTableEntry[i].peer2.commID == m_localCommunity))
        continue;
      else if (socialTableEntry[i].socialTieValue > equation) {
        if (socialTableEntry[i].peer1.commID != m_localCommunity)
          FringeNodeSet.push_back (socialTableEntry[i].peer1.ID);
        else
          FringeNodeSet.push_back (socialTableEntry[i].peer2.ID);
      }
      else if (socialTableEntry[i].socialTieValue <= equation)
        continue;  
  }
  return FringeNodeSet;
}

} // namespace ns3
