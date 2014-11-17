#include "ns3/log.h"
#include "ns3/interest-manager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("InterestManager");

InterestManager::InterestManager ()
{
    m_size = 0;
    m_array = new InterestEntry[1000];
}

InterestManager::~InterestManager ()
{
    delete [] m_array;
}

void
InterestManager::Insert(InterestEntry entry)
{
    if (m_size == 1000 || Exist(entry) )
    {
        return;
    }
    
    m_array[m_size] = entry;
    m_size++;
}

uint32_t
InterestManager::GetInterestArraySize() const
{
    return m_size;
}

bool
InterestManager::Exist(InterestEntry entry) const
{
    for (uint32_t i=0; i<m_size; ++i)
    {
        if (m_array[i].requesterId.IsEqual(entry.requesterId) &&
            m_array[i].broadcastId == entry.broadcastId)
        {
            return true;
        }
    }
    return false;
}



}
