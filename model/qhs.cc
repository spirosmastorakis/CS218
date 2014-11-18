#include "ns3/qhs.h"

namespace ns3 {

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
