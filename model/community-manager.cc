#include "ns3/log.h"
#include "ns3/community-manager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CommunityManager");

CommunityManager::CommunityManager ()
{
    m_array = new uint32_t[1000];
    m_size = 0;
}

CommunityManager::~CommunityManager ()
{
    delete [] m_array;
}
    
void
CommunityManager::Insert(uint32_t communityId)
{
    if (m_size == 1000)
    {
        return;
    }
    
    m_array[m_size] = communityId;
    m_size++;
}
    
uint32_t *
CommunityManager::GetCommunityArray() const
{
    return m_array;
}

uint32_t
CommunityManager::GetCommunityArraySize() const
{
    return m_size;
}
    
bool
CommunityManager::Exist(uint32_t communityId) const
{
    for (uint32_t i=0; i<m_size; ++i)
    {
        if (m_array[i] == communityId)
        {
            return true;
        }
    }
    return false;
}


}
