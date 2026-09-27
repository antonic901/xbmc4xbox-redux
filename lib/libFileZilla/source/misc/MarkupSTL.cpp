/*
 *  Copyright (C) 2023-2026 Team Xodi
 *  This file is part of Xodi - https://xodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "stdafx.h"
#include "MarkupSTL.h"

CMarkupSTL::CMarkupSTL()
  : m_parent(NULL), m_elem(NULL), m_child(NULL)
{
}

CMarkupSTL::CMarkupSTL(const char* szDoc)
  : m_parent(NULL), m_elem(NULL), m_child(NULL)
{
  SetDoc(szDoc);
}

CMarkupSTL::CMarkupSTL(const CMarkupSTL& markup)
  : m_parent(NULL), m_elem(NULL), m_child(NULL)
{
  CopyFrom(markup);
}

void CMarkupSTL::operator=(const CMarkupSTL& markup)
{
  if (this != &markup)
    CopyFrom(markup);
}

void CMarkupSTL::CopyFrom(const CMarkupSTL& markup)
{
  const std::string xml = markup.GetDoc();
  m_doc.Clear();
  m_savedPos.clear();
  m_error.clear();
  ResetPos();

  if (!xml.empty() && !m_doc.Parse(xml, TIXML_ENCODING_UTF8))
    m_error = m_doc.ErrorDesc() ? m_doc.ErrorDesc() : "XML parse error";
}

bool CMarkupSTL::SetDoc(const char* szDoc)
{
  m_doc.Clear();
  m_savedPos.clear();
  m_error.clear();
  ResetPos();

  if (!szDoc || !*szDoc)
    return false;

  if (!m_doc.Parse(szDoc, TIXML_ENCODING_UTF8))
  {
    m_error = m_doc.ErrorDesc() ? m_doc.ErrorDesc() : "XML parse error";
    return false;
  }
  return m_doc.RootElement() != NULL;
}

bool CMarkupSTL::Load(const char* szFileName)
{
  m_doc.Clear();
  m_savedPos.clear();
  m_error.clear();
  ResetPos();

  if (!szFileName || !m_doc.LoadFile(szFileName, TIXML_ENCODING_UTF8))
  {
    m_error = m_doc.ErrorDesc() ? m_doc.ErrorDesc() : "Unable to load XML file";
    return false;
  }
  return m_doc.RootElement() != NULL;
}

bool CMarkupSTL::Save(const char* szFileName)
{
  if (!szFileName)
    return false;

  if (!m_doc.SaveFile(szFileName))
  {
    m_error = "Unable to save XML file";
    return false;
  }
  return true;
}

bool CMarkupSTL::IsWellFormed()
{
  return m_doc.RootElement() != NULL && !m_doc.Error();
}

std::string CMarkupSTL::GetDoc() const
{
  TiXmlPrinter printer;
  m_doc.Accept(&printer);
  return printer.CStr() ? printer.CStr() : "";
}

std::string CMarkupSTL::GetError() const
{
  if (!m_error.empty())
    return m_error;
  return m_doc.ErrorDesc() ? m_doc.ErrorDesc() : "";
}

TiXmlElement* CMarkupSTL::FirstElement(TiXmlNode* parent)
{
  if (parent)
    return parent->FirstChildElement();
  return m_doc.RootElement();
}

TiXmlElement* CMarkupSTL::NextElement(TiXmlElement* current) const
{
  return current ? current->NextSiblingElement() : NULL;
}

TiXmlElement* CMarkupSTL::FindNext(TiXmlNode* parent, TiXmlElement* current, const char* name)
{
  TiXmlElement* elem = current ? NextElement(current) : FirstElement(parent);
  while (elem)
  {
    if (!name || !*name || strcmp(elem->Value(), name) == 0)
      return elem;
    elem = elem->NextSiblingElement();
  }
  return NULL;
}

bool CMarkupSTL::FindElem(const char* szName)
{
  TiXmlElement* found = FindNext(m_parent, m_elem, szName);
  if (!found)
    return false;

  m_elem = found;
  m_child = NULL;
  return true;
}

bool CMarkupSTL::FindChildElem(const char* szName)
{
  // Preserve Markup 6.3 shorthand: with no current main position, select the
  // first main element before looking for its child.
  if (!m_elem && !FindElem())
    return false;

  TiXmlElement* found = FindNext(m_elem, m_child, szName);
  if (!found)
    return false;

  m_child = found;
  return true;
}

bool CMarkupSTL::IntoElem()
{
  if (!m_elem)
    return false;

  m_parent = m_elem;
  m_elem = m_child;
  m_child = NULL;

  return true;
}

bool CMarkupSTL::OutOfElem()
{
  if (!m_parent)
    return false;

  TiXmlElement* child = m_elem;
  TiXmlElement* elem = m_parent;

  TiXmlNode* node = elem->Parent();

  m_parent = node ? node->ToElement() : NULL;
  m_elem = elem;
  m_child = child;

  return true;
}

void CMarkupSTL::ResetPos()
{
  m_parent = NULL;
  m_elem = NULL;
  m_child = NULL;
}

void CMarkupSTL::ResetMainPos()
{
  m_elem = NULL;
  m_child = NULL;
}

void CMarkupSTL::ResetChildPos()
{
  m_child = NULL;
}

std::string CMarkupSTL::GetTagName() const
{
  return m_elem && m_elem->Value() ? m_elem->Value() : "";
}

std::string CMarkupSTL::GetChildTagName() const
{
  return m_child && m_child->Value() ? m_child->Value() : "";
}

std::string CMarkupSTL::ElementText(const TiXmlElement* elem)
{
  if (!elem)
    return "";

  // CMarkup GetData() returns data only when the element has no child elements.
  if (elem->FirstChildElement())
    return "";

  const TiXmlNode* node = elem->FirstChild();
  if (!node)
    return "";

  const TiXmlText* text = node->ToText();
  if (!text || !text->Value())
    return "";

  return text->Value();
}

std::string CMarkupSTL::GetData() const
{
  return ElementText(m_elem);
}

std::string CMarkupSTL::GetChildData() const
{
  return ElementText(m_child);
}

std::string CMarkupSTL::GetAttrib(const char* szAttrib) const
{
  if (!m_elem || !szAttrib)
    return "";
  const char* value = m_elem->Attribute(szAttrib);
  return value ? value : "";
}

std::string CMarkupSTL::GetChildAttrib(const char* szAttrib) const
{
  if (!m_child || !szAttrib)
    return "";
  const char* value = m_child->Attribute(szAttrib);
  return value ? value : "";
}

std::string CMarkupSTL::GetAttribName(int n) const
{
  if (!m_elem || n < 0)
    return "";

  const TiXmlAttribute* attr = m_elem->FirstAttribute();
  while (attr && n-- > 0)
    attr = attr->Next();
  return attr && attr->Name() ? attr->Name() : "";
}

std::string CMarkupSTL::SerializeNode(const TiXmlNode* node)
{
  if (!node)
    return "";
  TiXmlPrinter printer;
  node->Accept(&printer);
  return printer.CStr() ? printer.CStr() : "";
}

std::string CMarkupSTL::GetChildSubDoc() const
{
  return SerializeNode(m_child);
}

bool CMarkupSTL::SetElementData(TiXmlElement* elem, const char* data, int cdata)
{
  if (!elem)
    return false;

  // Match old CMarkup behavior: SetData fails when child elements exist.
  if (elem->FirstChildElement())
    return false;

  while (elem->FirstChild())
    elem->RemoveChild(elem->FirstChild());

  TiXmlText text(data ? data : "");
  if (cdata)
    text.SetCDATA(true);
  return elem->InsertEndChild(text) != NULL;
}

bool CMarkupSTL::SetData(const char* szData, int nCDATA)
{
  return SetElementData(m_elem, szData, nCDATA);
}

bool CMarkupSTL::SetChildData(const char* szData, int nCDATA)
{
  return SetElementData(m_child, szData, nCDATA);
}

bool CMarkupSTL::SetElementAttrib(TiXmlElement* elem, const char* name, const char* value)
{
  if (!elem || !name)
    return false;
  elem->SetAttribute(name, value ? value : "");
  return true;
}

bool CMarkupSTL::SetElementAttrib(TiXmlElement* elem, const char* name, int value)
{
  if (!elem || !name)
    return false;
  elem->SetAttribute(name, value);
  return true;
}

bool CMarkupSTL::SetAttrib(const char* szAttrib, const char* szValue)
{
  return SetElementAttrib(m_elem, szAttrib, szValue);
}

bool CMarkupSTL::SetAttrib(const char* szAttrib, int nValue)
{
  return SetElementAttrib(m_elem, szAttrib, nValue);
}

bool CMarkupSTL::SetChildAttrib(const char* szAttrib, const char* szValue)
{
  return SetElementAttrib(m_child, szAttrib, szValue);
}

bool CMarkupSTL::SetChildAttrib(const char* szAttrib, int nValue)
{
  return SetElementAttrib(m_child, szAttrib, nValue);
}

bool CMarkupSTL::AddElement(const char* name, const char* data, bool insert, bool child)
{
  if (!name || !*name)
    return false;

  TiXmlElement element(name);
  if (data && *data)
  {
    TiXmlText text(data);
    element.InsertEndChild(text);
  }

  TiXmlNode* container;
  TiXmlElement* relative;

  if (child)
  {
    if (!m_elem)
      return false;
    container = m_elem;
    relative = m_child;
  }
  else
  {
    container = m_parent ? static_cast<TiXmlNode*>(m_parent) : static_cast<TiXmlNode*>(&m_doc);
    relative = m_elem;

    // A document may only have one root element.
    if (!m_parent && m_doc.RootElement() && !relative)
      return false;
  }

  TiXmlNode* added = NULL;
  if (insert)
  {
    if (relative)
      added = container->InsertBeforeChild(relative, element);
    else if (container->FirstChild())
      added = container->InsertBeforeChild(container->FirstChild(), element);
    else
      added = container->InsertEndChild(element);
  }
  else
  {
    if (relative)
      added = container->InsertAfterChild(relative, element);
    else
      added = container->InsertEndChild(element);
  }

  TiXmlElement* addedElem = added ? added->ToElement() : NULL;
  if (!addedElem)
    return false;

  if (child)
    m_child = addedElem;
  else
  {
    m_elem = addedElem;
    m_child = NULL;
  }
  return true;
}

bool CMarkupSTL::AddElem(const char* szName, const char* szData)
{
  return AddElement(szName, szData, false, false);
}

bool CMarkupSTL::InsertElem(const char* szName, const char* szData)
{
  return AddElement(szName, szData, true, false);
}

bool CMarkupSTL::AddChildElem(const char* szName, const char* szData)
{
  return AddElement(szName, szData, false, true);
}

bool CMarkupSTL::InsertChildElem(const char* szName, const char* szData)
{
  return AddElement(szName, szData, true, true);
}

bool CMarkupSTL::AddSubDoc(const char* subDoc, bool insert, bool child)
{
  if (!subDoc || !*subDoc)
    return false;

  CXBMCTinyXML temp;
  if (!temp.Parse(subDoc, TIXML_ENCODING_UTF8) || !temp.RootElement())
    return false;

  TiXmlElement* source = temp.RootElement();
  TiXmlNode* container;
  TiXmlElement* relative;

  if (child)
  {
    if (!m_elem)
      return false;
    container = m_elem;
    relative = m_child;
  }
  else
  {
    container = m_parent ? static_cast<TiXmlNode*>(m_parent) : static_cast<TiXmlNode*>(&m_doc);
    relative = m_elem;
  }

  TiXmlNode* added = NULL;
  if (insert)
  {
    if (relative)
      added = container->InsertBeforeChild(relative, *source);
    else if (container->FirstChild())
      added = container->InsertBeforeChild(container->FirstChild(), *source);
    else
      added = container->InsertEndChild(*source);
  }
  else
  {
    if (relative)
      added = container->InsertAfterChild(relative, *source);
    else
      added = container->InsertEndChild(*source);
  }

  TiXmlElement* addedElem = added ? added->ToElement() : NULL;
  if (!addedElem)
    return false;

  if (child)
    m_child = addedElem;
  else
  {
    m_elem = addedElem;
    m_child = NULL;
  }
  return true;
}

bool CMarkupSTL::AddChildSubDoc(const char* szSubDoc)
{
  return AddSubDoc(szSubDoc, false, true);
}

bool CMarkupSTL::InsertChildSubDoc(const char* szSubDoc)
{
  return AddSubDoc(szSubDoc, true, true);
}

bool CMarkupSTL::RemoveChildElem()
{
  if (!m_elem || !m_child)
    return false;

  TiXmlNode* previous = m_child->PreviousSibling();
  while (previous && !previous->ToElement())
    previous = previous->PreviousSibling();

  if (!m_elem->RemoveChild(m_child))
    return false;

  m_child = previous ? previous->ToElement() : NULL;
  m_savedPos.clear();
  return true;
}

bool CMarkupSTL::SavePos(const char* szPosName)
{
  const std::string name = szPosName ? szPosName : "";
  SavedPos pos;
  pos.parent = m_parent;
  pos.elem = m_elem;
  pos.child = m_child;
  m_savedPos[name] = pos;
  return true;
}

bool CMarkupSTL::RestorePos(const char* szPosName)
{
  const std::string name = szPosName ? szPosName : "";
  std::map<std::string, SavedPos>::iterator it = m_savedPos.find(name);
  if (it == m_savedPos.end())
    return false;

  m_parent = it->second.parent;
  m_elem = it->second.elem;
  m_child = it->second.child;
  return true;
}

bool CMarkupSTL::GetOffsets(int& nStart, int& nEnd) const
{
  nStart = 0;
  nEnd = 0;
  if (!m_elem)
    return false;

  const std::string doc = GetDoc();
  const std::string elem = SerializeNode(m_elem);
  if (elem.empty())
    return false;

  const std::string::size_type pos = doc.find(elem);
  if (pos == std::string::npos)
    return false;

  nStart = static_cast<int>(pos);
  nEnd = static_cast<int>(pos + elem.length() - 1);
  return true;
}
