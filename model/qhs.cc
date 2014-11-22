#include "ns3/qhs.h"
#include "ns3/log.h"
#include "ns3/ptr.h"

NS_LOG_COMPONENT_DEFINE ("qhs");

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

void
QHS::UpdateFringeNodeSet (SocialTableEntry *socialTieTable, uint32_t socialTieTableSize, std::vector<Ipv4Address> &FringeNodeSet)
{ 
  FringeNodeSet.clear ();
  double sum = 0;
  int number = 0;
  double max = 0;
  //SocialTableEntry *socialTableEntry = (SocialTableEntry *) (*socialTieTable);
  SocialTableEntry *socialTableEntry = (SocialTableEntry *) (socialTieTable);
  for (int i = 0 ; i < int (socialTieTableSize); i++) {
    NS_LOG_DEBUG ("Iteration: " << i);
    NS_LOG_DEBUG ("Social Tie value " << socialTableEntry[i].socialTieValue);
    NS_LOG_DEBUG ("peer1 " << socialTableEntry[i].peer1.commID);
    NS_LOG_DEBUG ("peer2 " << socialTableEntry[i].peer2.commID);
    if ((socialTableEntry[i].peer1.commID == m_localCommunity) and (socialTableEntry[i].peer2.commID == m_localCommunity)) 
      continue;
    else {
      sum += socialTableEntry[i].socialTieValue;
      number++; 
      NS_LOG_DEBUG ("Num of nodes " << number << " Iteration: " << i);
      if (max < socialTableEntry[i].socialTieValue)
        max = socialTableEntry[i].socialTieValue;
    }
    NS_LOG_DEBUG ("Max value " << max);
  }
  if (number > 0) {
    NS_LOG_DEBUG ("I am in push back fringe nodes phase");
    double avg = sum / number;
    float b = 0.5;
    double equation = avg + b * (max - avg);
    for (int i = 0 ; i < int (socialTieTableSize); i++) {
        if ((socialTableEntry[i].peer1.commID == m_localCommunity) and (socialTableEntry[i].peer2.commID == m_localCommunity))
          continue;
        else if (socialTableEntry[i].socialTieValue > equation) {
          if (socialTableEntry[i].peer1.commID != m_localCommunity) {
            FringeNodeSet.push_back (socialTableEntry[i].peer1.ID);
	    NS_LOG_DEBUG ("Node added");
          }
          else {
            FringeNodeSet.push_back (socialTableEntry[i].peer2.ID);
            NS_LOG_DEBUG ("Node added");
          }
        }
        else if (socialTableEntry[i].socialTieValue <= equation)
          continue;  
    }
  }
  NS_LOG_DEBUG ("Fringe node set size " << FringeNodeSet.size());
}

} // namespace ns3
