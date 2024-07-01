/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "EventStreamDetail.h"
#include "JobManager.h"
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include <algorithm>
#include <memory>
#include <vector>
#include <boost/function.hpp>


template<typename Event>
class CEventStream
{
public:

  template<typename A>
  void Subscribe(A* owner, void (A::*fn)(const Event&))
  {
    boost::shared_ptr<detail::CSubscription<Event, A> > subscription = boost::make_shared<detail::CSubscription<Event, A> >(owner, fn);
    CSingleLock lock(m_criticalSection);
    m_subscriptions.push_back(boost::move(subscription));
  }

  template<typename A>
  void Unsubscribe(A* obj)
  {
    std::vector<boost::shared_ptr<detail::ISubscription<Event> > > toCancel;
    {
      CSingleLock lock(m_criticalSection);
      std::vector<boost::shared_ptr<detail::ISubscription<Event> > >::iterator it = m_subscriptions.begin();
      while (it != m_subscriptions.end())
      {
        if ((*it)->IsOwnedBy(obj))
        {
          toCancel.push_back(*it);
          it = m_subscriptions.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
    for (std::vector<boost::shared_ptr<detail::ISubscription<Event> > >::const_iterator it = toCancel.begin(); it != toCancel.end(); ++it)
      (*it)->Cancel();
  }

protected:
  std::vector<boost::shared_ptr<detail::ISubscription<Event> > > m_subscriptions;
  CCriticalSection m_criticalSection;
};


template<typename Event>
class CEventSource : public CEventStream<Event>
{
public:
  template<typename A>
  void Publish(A event)
  {
    CSingleLock lock(this->m_criticalSection);
    std::vector<boost::shared_ptr<detail::ISubscription<Event> > >& subscriptions = this->m_subscriptions;
    boost::function<void()> task = boost::bind(&CEventSource::HandleEvent, this, subscriptions, event);
    lock.Leave();
    CJobManager::GetInstance().Submit(boost::move(task));
  }

private:
  void HandleEvent(std::vector<boost::shared_ptr<detail::ISubscription<Event> > >& subscriptions, Event event)
  {
    for (std::vector<boost::shared_ptr<detail::ISubscription<Event> > >::const_iterator it = subscriptions.begin(); it != subscriptions.end(); ++it)
      (*it)->HandleEvent(event);
  }
};
