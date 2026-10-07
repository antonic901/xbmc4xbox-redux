/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TagLoaderTagLib.h"

#include "DllTagLib.h"
#include "MusicInfoTag.h"
#include "ReplayGain.h"
#include "ServiceBroker.h"
#include "TagLibVFSStream.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/EmbeddedArt.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace MUSIC_INFO;

namespace
{
struct TagLibContext
{
  CMusicInfoTag* tag;
  EmbeddedArt* art;
};
} // namespace

// Callbacks always catch locally: MSVC exceptions must never unwind through
// the MinGW DLL (nor may DLL exceptions unwind through these callbacks).
int XBMC_TAGLIB_CALL CTagLoaderTagLib::OnQuery(void* opaque, unsigned int field)
{
  try
  {
    CMusicInfoTag& tag = *static_cast<TagLibContext*>(opaque)->tag;
    switch (field)
    {
      case XBMC_TAGLIB_ARTIST_EMPTY:
        return tag.GetArtist().empty() ? 1 : 0;
      case XBMC_TAGLIB_USER_RATING:
        return tag.GetUserrating();
      default:
        return -1;
    }
  }
  catch (...)
  {
    return -1;
  }
}

int XBMC_TAGLIB_CALL CTagLoaderTagLib::OnEvent(void* opaque, const XbmcTagLibEvent* event)
{
  try
  {
    if (!event || (event->count && !event->values))
      return 0;
    TagLibContext& context = *static_cast<TagLibContext*>(opaque);
    CMusicInfoTag& tag = *context.tag;
    std::vector<std::string> values;
    for (unsigned int i = 0; i < event->count; ++i)
      values.push_back(event->values[i] ? event->values[i] : "");
    const std::string value = values.empty() ? std::string() : values[0];
    switch (event->field)
    {
      case XBMC_TAGLIB_SetTitle:
        tag.SetTitle(value);
        break;
      case XBMC_TAGLIB_SetAlbum:
        tag.SetAlbum(value);
        break;
      case XBMC_TAGLIB_SetComment:
        tag.SetComment(value);
        break;
      case XBMC_TAGLIB_SetMood:
        tag.SetMood(value);
        break;
      case XBMC_TAGLIB_SetRecordLabel:
        tag.SetRecordLabel(value);
        break;
      case XBMC_TAGLIB_SetReleaseDate:
        tag.SetReleaseDate(value);
        break;
      case XBMC_TAGLIB_SetOriginalDate:
        tag.SetOriginalDate(value);
        break;
      case XBMC_TAGLIB_AddReleaseDate:
        tag.AddReleaseDate(value);
        break;
      case XBMC_TAGLIB_AddOriginalDate:
        tag.AddOriginalDate(value);
        break;
      case XBMC_TAGLIB_SetDiscSubtitle:
        tag.SetDiscSubtitle(value);
        break;
      case XBMC_TAGLIB_SetMusicBrainzAlbumID:
        tag.SetMusicBrainzAlbumID(value);
        break;
      case XBMC_TAGLIB_SetMusicBrainzReleaseGroupID:
        tag.SetMusicBrainzReleaseGroupID(value);
        break;
      case XBMC_TAGLIB_SetMusicBrainzTrackID:
        tag.SetMusicBrainzTrackID(value);
        break;
      case XBMC_TAGLIB_SetAlbumReleaseStatus:
        tag.SetAlbumReleaseStatus(value);
        break;
      case XBMC_TAGLIB_SetLyrics:
        tag.SetLyrics(value);
        break;
      case XBMC_TAGLIB_SetCueSheet:
        tag.SetCueSheet(value);
        break;
      case XBMC_TAGLIB_SetTrackNumber:
        tag.SetTrackNumber(event->number);
        break;
      case XBMC_TAGLIB_SetDiscNumber:
        tag.SetDiscNumber(event->number);
        break;
      case XBMC_TAGLIB_SetYear:
        tag.SetYear(event->number);
        break;
      case XBMC_TAGLIB_SetCompilation:
        tag.SetCompilation(event->number);
        break;
      case XBMC_TAGLIB_SetBPM:
        tag.SetBPM(event->number);
        break;
      case XBMC_TAGLIB_SetUserrating:
        tag.SetUserrating(event->number);
        break;
      case XBMC_TAGLIB_SetLoaded:
        tag.SetLoaded(event->number);
        break;
      case XBMC_TAGLIB_SetDuration:
        tag.SetDuration(event->number);
        break;
      case XBMC_TAGLIB_SetBitRate:
        tag.SetBitRate(event->number);
        break;
      case XBMC_TAGLIB_SetNoOfChannels:
        tag.SetNoOfChannels(event->number);
        break;
      case XBMC_TAGLIB_SetSampleRate:
        tag.SetSampleRate(event->number);
        break;
      case XBMC_TAGLIB_SetArtist:
        SetArtist(tag, values);
        break;
      case XBMC_TAGLIB_SetArtistSort:
        SetArtistSort(tag, values);
        break;
      case XBMC_TAGLIB_SetArtistHints:
        SetArtistHints(tag, values);
        break;
      case XBMC_TAGLIB_SetAlbumArtist:
        SetAlbumArtist(tag, values);
        break;
      case XBMC_TAGLIB_SetAlbumArtistSort:
        SetAlbumArtistSort(tag, values);
        break;
      case XBMC_TAGLIB_SetAlbumArtistHints:
        SetAlbumArtistHints(tag, values);
        break;
      case XBMC_TAGLIB_SetComposerSort:
        SetComposerSort(tag, values);
        break;
      case XBMC_TAGLIB_SetGenre:
        SetGenre(tag, values);
        break;
      case XBMC_TAGLIB_SetReleaseType:
        SetReleaseType(tag, values);
        break;
      case XBMC_TAGLIB_AddArtistInstrument:
        AddArtistInstrument(tag, values);
        break;
      case XBMC_TAGLIB_SetMusicBrainzArtistID:
        tag.SetMusicBrainzArtistID(SplitMBID(values));
        break;
      case XBMC_TAGLIB_SetMusicBrainzAlbumArtistID:
        tag.SetMusicBrainzAlbumArtistID(SplitMBID(values));
        break;
      case XBMC_TAGLIB_AddArtistRole:
        if (!event->text)
          return 0;
        AddArtistRole(tag, event->text, values);
        break;
      case XBMC_TAGLIB_AddArtistRolePairs:
        AddArtistRole(tag, values);
        break;
      case XBMC_TAGLIB_SetReplayGain:
      {
        if (event->count != 4)
          return 0;
        ReplayGain gain;
        if (event->values[0])
          gain.ParseGain(ReplayGain::ALBUM, values[0]);
        if (event->values[1])
          gain.ParsePeak(ReplayGain::ALBUM, values[1]);
        if (event->values[2])
          gain.ParseGain(ReplayGain::TRACK, values[2]);
        if (event->values[3])
          gain.ParsePeak(ReplayGain::TRACK, values[3]);
        tag.SetReplayGain(gain);
        break;
      }
      case XBMC_TAGLIB_SetCoverArtInfo:
        if (!event->text)
          return 0;
        tag.SetCoverArtInfo(event->size, event->text);
        break;
      case XBMC_TAGLIB_SetCoverArt:
        if (!event->text || (event->size && !event->data))
          return 0;
        if (context.art)
          context.art->Set(event->data, event->size, event->text);
        break;
      case XBMC_TAGLIB_Log:
        if (event->text)
          CLog::Log(LOGDEBUG, "TagLib: %s", event->text);
        break;
      default:
        return 0;
    }
    return 1;
  }
  catch (...)
  {
    return 0;
  }
}

bool CTagLoaderTagLib::Load(const std::string& filename, CMusicInfoTag& tag, EmbeddedArt* art)
{
  return Load(filename, tag, "", art);
}

bool CTagLoaderTagLib::Load(const std::string& filename,
                            CMusicInfoTag& tag,
                            const std::string& fallbackFileExtension,
                            EmbeddedArt* art)
{
  std::string extension = URIUtils::GetExtension(filename);
  StringUtils::TrimLeft(extension, ".");
  if (extension.empty())
    extension = fallbackFileExtension;
  StringUtils::TrimLeft(extension, ".");
  StringUtils::ToLower(extension);
  if (extension.empty())
    return false;

  DllTagLib dll;
  if (!dll.Load())
  {
    CLog::Log(LOGERROR, "TagLib: cannot load Q:\\system\\libtag.dll with the XBMC C bridge");
    return false;
  }
  if (dll.xbmc_taglib_version() != XBMC_TAGLIB_ABI_VERSION)
  {
    CLog::Log(LOGERROR, "TagLib: incompatible XBMC bridge ABI version");
    return false;
  }

  try
  {
    TagLibVFSStream stream(filename);
    if (!stream.IsOpen())
      return false;

    // Apply metadata to a copy so a callback/I/O/parse failure cannot leave
    // partially populated tags or artwork in the caller's objects.
    CMusicInfoTag parsed(tag);
    EmbeddedArt parsedArt;
    if (art)
      parsedArt = *art;
    TagLibContext context = {&parsed, art ? &parsedArt : NULL};
    XbmcTagLibRequest request = {};
    request.size = sizeof(request);
    request.abi_version = XBMC_TAGLIB_ABI_VERSION;
    request.filename = filename.c_str();
    request.extension = extension.c_str();
    request.io = stream.GetCallbacks();
    request.callbacks.context = &context;
    request.callbacks.event = OnEvent;
    request.callbacks.query = OnQuery;
    request.prefer_ape =
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_prioritiseAPEv2tags;
    request.read_art = art != NULL;
    request.debug =
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel == LOG_LEVEL_MAX;
    char error[512] = {};
    if (!dll.xbmc_taglib_read(&request, error, sizeof(error)))
    {
      CLog::Log(LOGERROR, "TagLib: %s: %s", filename.c_str(), error);
      return false;
    }
    if (!parsed.GetTitle().empty() || !parsed.GetArtist().empty() || !parsed.GetAlbum().empty())
      parsed.SetLoaded();
    parsed.SetURL(filename);
    tag = parsed;
    if (art)
      *art = parsedArt;
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "TagLib: failed to read %s", filename.c_str());
    return false;
  }
}

void CTagLoaderTagLib::SetArtist(CMusicInfoTag& tag, const std::vector<std::string>& values)
{
  if (values.size() == 1)
    tag.SetArtist(values[0]);
  else
    // Fill both artist vector and artist desc from tag value.
    // Note desc may not be empty as it could have been set by previous parsing of ID3v2 before APE
    tag.SetArtist(values, true);
}

void CTagLoaderTagLib::SetArtistSort(CMusicInfoTag& tag, const std::vector<std::string>& values)
{
  // ARTISTSORT/TSOP tag is often a single string, when not take union of values
  if (values.size() == 1)
    tag.SetArtistSort(values[0]);
  else
    tag.SetArtistSort(StringUtils::Join(
        values,
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
}

void CTagLoaderTagLib::SetArtistHints(CMusicInfoTag& tag, const std::vector<std::string>& values)
{
  if (values.size() == 1)
    tag.SetMusicBrainzArtistHints(StringUtils::Split(
        values[0],
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
  else
    tag.SetMusicBrainzArtistHints(values);
}

std::vector<std::string> CTagLoaderTagLib::SplitMBID(const std::vector<std::string>& values)
{
  if (values.empty() || values.size() > 1)
    return values;

  // Picard, and other taggers use a heap of different separators.  We use a regexp to detect
  // MBIDs to make sure we hit them all...
  std::vector<std::string> ret;
  std::string value = values[0];
  StringUtils::ToLower(value);
  CRegExp reg;
  if (reg.RegComp(
          "([[:xdigit:]]{8}-[[:xdigit:]]{4}-[[:xdigit:]]{4}-[[:xdigit:]]{4}-[[:xdigit:]]{12})"))
  {
    int pos = -1;
    while ((pos = reg.RegFind(value, pos + 1)) > -1)
      ret.push_back(reg.GetMatch(1));
  }
  return ret;
}

void CTagLoaderTagLib::SetAlbumArtist(CMusicInfoTag& tag, const std::vector<std::string>& values)
{
  if (values.size() == 1)
    tag.SetAlbumArtist(values[0]);
  else
    // Fill both artist vector and artist desc from tag value.
    // Note desc may not be empty as it could have been set by previous parsing of ID3v2 before APE
    tag.SetAlbumArtist(values, true);
}

void CTagLoaderTagLib::SetAlbumArtistSort(CMusicInfoTag& tag,
                                          const std::vector<std::string>& values)
{
  // ALBUMARTISTSORT/TSOP tag is often a single string, when not take union of values
  if (values.size() == 1)
    tag.SetAlbumArtistSort(values[0]);
  else
    tag.SetAlbumArtistSort(StringUtils::Join(
        values,
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
}

void CTagLoaderTagLib::SetAlbumArtistHints(CMusicInfoTag& tag,
                                           const std::vector<std::string>& values)
{
  if (values.size() == 1)
    tag.SetMusicBrainzAlbumArtistHints(StringUtils::Split(
        values[0],
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
  else
    tag.SetMusicBrainzAlbumArtistHints(values);
}

void CTagLoaderTagLib::SetComposerSort(CMusicInfoTag& tag, const std::vector<std::string>& values)
{
  // COMPOSRSORT/TSOC tag is often a single string, when not take union of values
  if (values.size() == 1)
    tag.SetComposerSort(values[0]);
  else
    tag.SetComposerSort(StringUtils::Join(
        values,
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
}

void CTagLoaderTagLib::SetGenre(CMusicInfoTag& tag, const std::vector<std::string>& values)
{
  // Numeric ID3v1 genre identifiers have already been resolved inside libtag.
  if (values.size() == 1)
    tag.SetGenre(values[0], true);
  else
    tag.SetGenre(values, true);
}

void CTagLoaderTagLib::SetReleaseType(CMusicInfoTag& tag, const std::vector<std::string>& values)
{
  if (values.size() == 1)
    tag.SetMusicBrainzReleaseType(values[0]);
  else
    tag.SetMusicBrainzReleaseType(StringUtils::Join(
        values,
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
}

void CTagLoaderTagLib::AddArtistRole(CMusicInfoTag& tag,
                                     const std::string& strRole,
                                     const std::vector<std::string>& values)
{
  if (values.size() == 1)
    tag.AddArtistRole(strRole, values[0]);
  else
    tag.AddArtistRole(strRole, values);
}

void CTagLoaderTagLib::SetDiscSubtitle(CMusicInfoTag& tag, const std::vector<std::string>& values)
{
  if (values.size() == 1)
    tag.SetDiscSubtitle(values[0]);
  else
    tag.SetDiscSubtitle(std::string());
}

void CTagLoaderTagLib::AddArtistRole(CMusicInfoTag& tag, const std::vector<std::string>& values)
{
  // Values contains role, name pairs (as in ID3 standard for TIPL or TMCL tags)
  // Every odd entry is a function (e.g. Producer, Arranger etc.) or instrument (e.g. Orchestra, Vocal, Piano)
  // and every even is an artist or a comma delimited list of artists.

  if (values.size() % 2 != 0) // Must contain an even number of entries
    return;

  // Vector of possible separators
  const std::string separatorValues[] = {";", "/", ",", "&", " and "};
  const std::vector<std::string> separators(
      separatorValues, separatorValues + sizeof(separatorValues) / sizeof(separatorValues[0]));

  for (size_t i = 0; i + 1 < values.size(); i += 2)
  {
    std::vector<std::string> roles;
    //Split into individual roles
    roles = StringUtils::Split(values[i], separators);
    for (std::vector<std::string>::const_iterator it = roles.begin(); it != roles.end(); ++it)
    {
      std::string role = *it;
      StringUtils::Trim(role);
      //StringUtils::ToCapitalize(role);
      tag.AddArtistRole(role, StringUtils::Split(values[i + 1], ","));
    }
  }
}

void CTagLoaderTagLib::AddArtistInstrument(CMusicInfoTag& tag,
                                           const std::vector<std::string>& values)
{
  /* Values is a musician credits list, each entry is artist name followed by instrument (or function)
     e.g. violin, drums, background vocals, solo, orchestra etc. in brackets. This is how Picard uses
     the PERFORMER tag. Multiple instruments may be in one tag
     e.g "Pierre Marchand (bass, drum machine and hammond organ)",
     these will be separated into individual roles.
     If there is not a pair of brackets then role is "performer" by default, and the whole entry is
     taken as artist name.
  */
  // Vector of possible separators
  const std::string separatorValues[] = {";", "/", ",", "&", " and "};
  const std::vector<std::string> separators(
      separatorValues, separatorValues + sizeof(separatorValues) / sizeof(separatorValues[0]));

  for (size_t i = 0; i < values.size(); ++i)
  {
    std::vector<std::string> roles;
    std::string strArtist = values[i];
    size_t firstLim = values[i].find_first_of('(');
    size_t lastLim = values[i].find_last_of(')');
    if (lastLim != std::string::npos && firstLim != std::string::npos && firstLim < lastLim - 1)
    {
      //Pair of brackets with something between them
      strArtist.erase(firstLim, lastLim - firstLim + 1);
      std::string strRole = values[i].substr(firstLim + 1, lastLim - firstLim - 1);
      //Split into individual roles
      roles = StringUtils::Split(strRole, separators);
    }
    StringUtils::Trim(strArtist);
    if (roles.empty())
      tag.AddArtistRole("Performer", strArtist);
    else
      for (std::vector<std::string>::const_iterator it = roles.begin(); it != roles.end(); ++it)
      {
        std::string role = *it;
        StringUtils::Trim(role);
        //StringUtils::ToCapitalize(role);
        tag.AddArtistRole(role, strArtist);
      }
  }
}
