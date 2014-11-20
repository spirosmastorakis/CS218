#ifndef QHS_H
#define QHS_H

#include <map>
#include <utility>
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
  // originalTable is the social table before encounter
  // newTable is the social table of the encountered node
  // ID is the address of the encountered node
  uint32_t* UpdateSocialTable (uint32_t *originalTable, uint32_t originalSize,
			       uint32_t *newTable, uint32_t newSize,
			       Ipv4Address ID);

  std::vector<Ipv4Address>
  UpdateFringeNodeSet (uint32_t *socialTieTable, uint32_t socialTieTableSize);

private:
  std::vector<Ipv4Address> FringeNodeSet;
  uint32_t m_localCommunity;
};


} // namespace ns3

#endif 
