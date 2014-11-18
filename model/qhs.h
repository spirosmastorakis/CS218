#ifndef QHS_H
#define QHS_H

#include "ns3/social-network.h"
#include "ns3/relationship.h"

namespace ns3 {

class QHS {
public:
  QHS ();

  QHS (uint32_t localCommunity);

  ~QHS ();

  uint32_t
  GetLocalCommunity ();

  void
  SetLocalCommunity (uint32_t localCommunity);

  // Qiuhan: Add Update Table method

  std::vector<Ipv4Address>
  UpdateFringeNodeSet (uint32_t *socialTieTable, uint32_t socialTieTableSize);

private:
  std::vector<Ipv4Address> FringeNodeSet;
  uint32_t m_localCommunity;
};


} // namespace ns3

#endif 
