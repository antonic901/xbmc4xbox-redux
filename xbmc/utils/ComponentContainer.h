/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <cstddef>
#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include "commons/typeindex.h"
#include <map>
#include <utility>

//! \brief A generic container for components.
//! \details A component has to be derived from the BaseType.
//!          Only a single instance of each derived type can be registered.
//!          Intended use is through inheritance.
template<class BaseType>
class CComponentContainer
{
public:
  //! \brief Obtain a component.
  template<class T>
  boost::shared_ptr<T> GetComponent()
  {
    return boost::const_pointer_cast<T>(std::as_const(*this).template GetComponent<T>());
  }

  //! \brief Obtain a component.
  template<class T>
  boost::shared_ptr<const T> GetComponent() const
  {
    CSingleLock lock(m_critSection);
    std::map<XbmcCommons::type_index, boost::shared_ptr<BaseType> >::const_iterator it = m_components.find(XbmcCommons::type_index(typeid(T)));
    if (it != m_components.end())
      return boost::static_pointer_cast<const T>((*it).second);

    throw std::logic_error("ComponentContainer: Attempt to obtain non-existent component");
  }

  //! \brief Returns number of registered components.
  std::size_t size() const { return m_components.size(); }

protected:
  //! \brief Register a new component instance.
  void RegisterComponent(const boost::shared_ptr<BaseType>& component)
  {
    if (!component)
      return;

    // Note: Extra var needed to avoid clang warning
    // "Expression with side effects will be evaluated despite being used as an operand to 'typeid'"
    // https://stackoverflow.com/questions/46494928/clang-warning-on-expression-side-effects
    const BaseType& componentRef = *component;

    CSingleLock lock(m_critSection);
    m_components.insert(std::make_pair(XbmcCommons::type_index(typeid(componentRef)), component));
  }

  //! \brief Deregister a component.
  void DeregisterComponent(const std::type_info& typeInfo)
  {
    CSingleLock lock(m_critSection);
    m_components.erase(typeInfo);
  }

private:
  mutable CCriticalSection m_critSection; //!< Critical section for map updates
  std::map<XbmcCommons::type_index, boost::shared_ptr<BaseType> >
      m_components; //!< Map of components
};
