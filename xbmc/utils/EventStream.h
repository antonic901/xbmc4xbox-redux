/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "EventStreamDetail.h"
#include "JobManager.h"
#include "threads/CriticalSection.h"

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include <vector>


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
    for (std::vector<boost::shared_ptr<detail::ISubscription<Event> > >::const_iterator subscription = toCancel.begin(); subscription != toCancel.end(); ++subscription)
      (*subscription)->Cancel();
  }

protected:
  std::vector<boost::shared_ptr<detail::ISubscription<Event> > > m_subscriptions;
  CCriticalSection m_criticalSection;
};


template<typename Event>
class CEventSource : public CEventStream<Event>
{
public:
  explicit CEventSource() : m_queue(false, 1, CJob::PRIORITY_HIGH) {}

  template<typename A>
  void Publish(A event)
  {
    CSingleLock lock(this->m_criticalSection);
    std::vector<boost::shared_ptr<detail::ISubscription<Event> > >& subscriptions = this->m_subscriptions;
    boost::function<void()> task = boost::bind(&CEventSource::HandleEvent, this, subscriptions, event);
    lock.unlock();
    m_queue.Submit(boost::move(task));
  }

private:
  void HandleEvent(std::vector<boost::shared_ptr<detail::ISubscription<Event> > >& subscriptions, Event event)
  {
    for (std::vector<boost::shared_ptr<detail::ISubscription<Event> > >::const_iterator s = subscriptions.begin(); s != subscriptions.end(); ++s)
      (*s)->HandleEvent(event);
  }

  CJobQueue m_queue;
};

template<typename Event>
class CBlockingEventSource : public CEventStream<Event>
{
public:
  template<typename A>
  void HandleEvent(A event)
  {
    CSingleLock lock(this->m_criticalSection);
    for (std::vector<boost::shared_ptr<detail::ISubscription<Event> > >::const_iterator subscription = this->m_subscriptions.begin(); subscription != this->m_subscriptions.end(); ++subscription)
    {
      (*subscription)->HandleEvent(event);
    }
  }
};
