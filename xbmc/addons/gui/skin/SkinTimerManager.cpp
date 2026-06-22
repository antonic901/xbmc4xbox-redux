/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SkinTimerManager.h"

#include "GUIInfoManager.h"
#include "guilib/GUIAction.h"
#include "guilib/GUIComponent.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <boost/move/make_unique.hpp>

CSkinTimerManager::CSkinTimerManager(CGUIInfoManager& infoMgr) : m_infoMgr(infoMgr)
{
}

void CSkinTimerManager::LoadTimers(const std::string& path)
{
  CXBMCTinyXML doc;
  if (!doc.LoadFile(path))
  {
    CLog::Log(LOGWARNING, "Could not load timers file %s: %s (Line: %i)", path.c_str(), doc.ErrorDesc(),
               doc.ErrorRow());
    return;
  }

  TiXmlElement* root = doc.RootElement();
  if (!root || !StringUtils::EqualsNoCase(root->Value(), "timers"))
  {
    CLog::Log(LOGERROR, "Error loading timers file %s: Root element <timers> required.", path.c_str());
    return;
  }

  const TiXmlElement* timerNode = root->FirstChildElement("timer");
  while (timerNode)
  {
    LoadTimerInternal(timerNode);
    timerNode = timerNode->NextSiblingElement("timer");
  }
}

void CSkinTimerManager::LoadTimerInternal(const TiXmlElement* node)
{
  if ((!node->FirstChildElement("name") || !node->FirstChildElement("name")->FirstChild()))
  {
    CLog::Log(LOGERROR, "Missing required field 'name' for valid skin timer. Ignoring timer.");
    return;
  }

  std::string timerName = node->FirstChildElement("name")->FirstChild()->Value();
  if (TimerExists(timerName))
  {
    CLog::Log(LOGWARNING,
               "Ignoring timer with name {} - another timer with the same name already exists",
               timerName);
    return;
  }

  // timer start
  INFO::InfoPtr startInfo;
  bool resetOnStart(false);
  if (node->FirstChildElement("start") && node->FirstChildElement("start")->FirstChild())
  {
    startInfo = m_infoMgr.Register(node->FirstChildElement("start")->FirstChild()->Value());
    // check if timer needs to be reset after start
    if (node->FirstChildElement("start")->Attribute("reset") &&
        StringUtils::EqualsNoCase(node->FirstChildElement("start")->Attribute("reset"), "true"))
    {
      resetOnStart = true;
    }
  }

  // timer reset
  INFO::InfoPtr resetInfo;
  if (node->FirstChildElement("reset") && node->FirstChildElement("reset")->FirstChild())
  {
    resetInfo = m_infoMgr.Register(node->FirstChildElement("reset")->FirstChild()->Value());
  }
  // timer stop
  INFO::InfoPtr stopInfo;
  if (node->FirstChildElement("stop") && node->FirstChildElement("stop")->FirstChild())
  {
    stopInfo = m_infoMgr.Register(node->FirstChildElement("stop")->FirstChild()->Value());
  }

  // process onstart actions
  CGUIAction startActions;
  startActions.EnableSendThreadMessageMode();
  const TiXmlElement* onStartElement = node->FirstChildElement("onstart");
  while (onStartElement)
  {
    if (onStartElement->FirstChild())
    {
      const std::string conditionalActionAttribute =
          onStartElement->Attribute("condition") != NULL ? onStartElement->Attribute("condition")
                                                            : "";
      startActions.Append(CGUIAction::CExecutableAction(conditionalActionAttribute,
                                                        onStartElement->FirstChild()->Value()));
    }
    onStartElement = onStartElement->NextSiblingElement("onstart");
  }

  // process onstop actions
  CGUIAction stopActions;
  stopActions.EnableSendThreadMessageMode();
  const TiXmlElement* onStopElement = node->FirstChildElement("onstop");
  while (onStopElement)
  {
    if (onStopElement->FirstChild())
    {
      const std::string conditionalActionAttribute =
          onStopElement->Attribute("condition") != NULL ? onStopElement->Attribute("condition")
                                                           : "";
      stopActions.Append(CGUIAction::CExecutableAction(conditionalActionAttribute,
                                                       onStopElement->FirstChild()->Value()));
    }
    onStopElement = onStopElement->NextSiblingElement("onstop");
  }

  m_timers[timerName] = boost::movelib::make_unique<CSkinTimer>(CSkinTimer(
      timerName, startInfo, resetInfo, stopInfo, startActions, stopActions, resetOnStart));
}

bool CSkinTimerManager::TimerIsRunning(const std::string& timer) const
{
  boost::unordered_map<std::string, boost::movelib::unique_ptr<CSkinTimer> >::const_iterator iter = m_timers.find(timer);
  if (iter != m_timers.end())
  {
    return iter->second->IsRunning();
  }
  CLog::Log(LOGERROR, "Couldn't find Skin Timer with name: %s", timer.c_str());
  return false;
}

float CSkinTimerManager::GetTimerElapsedSeconds(const std::string& timer) const
{
  boost::unordered_map<std::string, boost::movelib::unique_ptr<CSkinTimer> >::const_iterator iter = m_timers.find(timer);
  if (iter != m_timers.end())
  {
    return iter->second->GetElapsedSeconds();
  }
  CLog::Log(LOGERROR, "Couldn't find Skin Timer with name: %s", timer.c_str());
  return 0;
}

void CSkinTimerManager::TimerStart(const std::string& timer) const
{
  boost::unordered_map<std::string, boost::movelib::unique_ptr<CSkinTimer> >::const_iterator iter = m_timers.find(timer);
  if (iter != m_timers.end())
  {
    return iter->second->Start();
  }
  CLog::Log(LOGERROR, "Couldn't find Skin Timer with name: %s", timer.c_str());
}

void CSkinTimerManager::TimerStop(const std::string& timer) const
{
  boost::unordered_map<std::string, boost::movelib::unique_ptr<CSkinTimer> >::const_iterator iter = m_timers.find(timer);
  if (iter != m_timers.end())
  {
    return iter->second->Stop();
  }
  CLog::Log(LOGERROR, "Couldn't find Skin Timer with name: %s", timer.c_str());
}

size_t CSkinTimerManager::GetTimerCount() const
{
  return m_timers.size();
}

bool CSkinTimerManager::TimerExists(const std::string& timer) const
{
  return m_timers.count(timer) != 0;
}

boost::movelib::unique_ptr<CSkinTimer> CSkinTimerManager::GrabTimer(const std::string& timer)
{
  boost::unordered_map<std::string, boost::movelib::unique_ptr<CSkinTimer> >::iterator iter = m_timers.find(timer);
  if (iter != m_timers.end())
  {
    boost::movelib::unique_ptr<CSkinTimer> timerInstance = boost::move(iter->second);
    m_timers.erase(iter);
    return boost::move(timerInstance);
  }
  return boost::movelib::unique_ptr<CSkinTimer>();
}

void CSkinTimerManager::Stop()
{
  // skintimers, as infomanager clients register info conditions/expressions in the infomanager.
  // The infomanager is linked to skins, being initialized or cleared when
  // skins are loaded (or unloaded). All the registered boolean conditions from
  // skin timers will end up being removed when the skin is unloaded. However, to
  // self-contain this component unregister them all here.
  for (boost::unordered_map<std::string, boost::movelib::unique_ptr<CSkinTimer> >::const_iterator key = m_timers.begin(); key != m_timers.end(); ++key)
  {
    const boost::movelib::unique_ptr<CSkinTimer>::pointer timer = key->second.get();
    if (timer->GetStartCondition())
    {
      m_infoMgr.UnRegister(timer->GetStartCondition());
    }
    if (timer->GetStopCondition())
    {
      m_infoMgr.UnRegister(timer->GetStopCondition());
    }
    if (timer->GetResetCondition())
    {
      m_infoMgr.UnRegister(timer->GetResetCondition());
    }
  }
  m_timers.clear();
}

void CSkinTimerManager::Process()
{
  for (boost::unordered_map<std::string, boost::movelib::unique_ptr<CSkinTimer> >::const_iterator key = m_timers.begin(); key != m_timers.end(); ++key)
  {
    const boost::movelib::unique_ptr<CSkinTimer>::pointer timer = key->second.get();
    if (!timer->IsRunning() && timer->VerifyStartCondition())
    {
      timer->Start();
    }
    else if (timer->IsRunning() && timer->VerifyStopCondition())
    {
      timer->Stop();
    }
    if (timer->GetElapsedSeconds() > 0 && timer->VerifyResetCondition())
    {
      timer->Reset();
    }
  }
}
