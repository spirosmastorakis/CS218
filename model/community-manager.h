
#ifndef COMMUNITY_MANAGER_H
#define COMMUNITY_MANAGER_H

#include "ns3/ipv4-address.h"

//#define MAX_SIZE 1000 =>cause a compiler error

using namespace std;

namespace ns3
{

class CommunityManager
{
public:    
    CommunityManager ();
    virtual ~CommunityManager ();
    
    void Insert(uint32_t communityId);
    uint32_t *GetCommunityArray() const;
    uint32_t GetCommunityArraySize() const;
    
    //check if community id exists
    bool Exist(uint32_t communityId) const;
    
private:
    uint32_t *m_array; //contain only unique entry
    uint32_t m_size;

};

}	// namespace ns3

#endif /* COMMUNITY_MANAGER_H */
