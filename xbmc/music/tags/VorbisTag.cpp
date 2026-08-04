/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "VorbisTag.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"

using namespace MUSIC_INFO;

CVorbisTag::CVorbisTag()
{

}

CVorbisTag::~CVorbisTag()
{

}

int CVorbisTag::ParseTagEntry(std::string& strTagEntry)
{
  std::string strTagValue;
  std::string strTagType;

  // Split tag entry like ARTIST=Sublime
  SplitEntry( strTagEntry, strTagType, strTagValue);

  // Save tag entry to members

  CMusicInfoTag& tag=m_musicInfoTag;

  if ( strTagType == "ARTIST" )
  {
    tag.AppendArtist(strTagValue);
    tag.SetLoaded();
  }

  if ( strTagType == "ALBUMARTIST" || strTagType == "ALBUM ARTIST" || strTagType == "ENSEMBLE")
  {
    tag.AppendAlbumArtist(strTagValue);
    tag.SetLoaded();
  }

  if ( strTagType == "TITLE" )
  {
    tag.SetTitle(strTagValue);
    tag.SetLoaded();
  }

  if ( strTagType == "ALBUM" )
  {
    tag.SetAlbum(strTagValue);
    tag.SetLoaded();
  }

  if ( strTagType == "TRACKNUMBER" )
  {
    tag.SetTrackNumber(atoi(strTagValue.c_str()));
  }

  if ( strTagType == "DISCNUMBER" )
  {
    tag.SetDiscNumber(atoi(strTagValue.c_str()));
  }

  if ( strTagType == "DATE" )
  {
    tag.SetYear(atoi(strTagValue.c_str()));
  }

  if ( strTagType == "GENRE" )
  {
    tag.AppendGenre(strTagValue);
  }

  if ( strTagType == "MUSICBRAINZ_TRACKID" )
  {
    tag.SetMusicBrainzTrackID(strTagValue);
  }

  if ( strTagType == "MUSICBRAINZ_ARTISTID" )
  {
    tag.SetMusicBrainzArtistID(StringUtils::Split(strTagValue, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
  }

  if ( strTagType == "MUSICBRAINZ_ALBUMID" )
  {
    tag.SetMusicBrainzAlbumID(strTagValue);
  }

  if ( strTagType == "MUSICBRAINZ_ALBUMARTISTID" )
  {
    tag.SetMusicBrainzAlbumArtistID(StringUtils::Split(strTagValue, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
  }

  if ( strTagType == "COMMENT" || strTagType == "DESCRIPTION" )
    tag.SetComment(strTagValue);

  if ( strTagType == "LYRICS" )
    tag.SetLyrics(strTagValue);


  if ( strTagType == "RATING" && strTagValue.length() == 1 && strTagValue[0] > '0' && strTagValue[0] < '6')
    tag.SetRating(strTagValue[0]);

  //  Get new style replay gain info
  if (strTagType=="REPLAYGAIN_TRACK_GAIN")
  {
    m_replayGain.iTrackGain = (int)(atof(strTagValue.c_str()) * 100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
  }
  else if (strTagType=="REPLAYGAIN_TRACK_PEAK")
  {
    m_replayGain.fTrackPeak = (float)atof(strTagValue.c_str());
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
  }
  else if (strTagType=="REPLAYGAIN_ALBUM_GAIN")
  {
    m_replayGain.iAlbumGain = (int)(atof(strTagValue.c_str()) * 100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
  }
  else if (strTagType=="REPLAYGAIN_ALBUM_PEAK")
  {
    m_replayGain.fAlbumPeak = (float)atof(strTagValue.c_str());
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
  }

  //  Get old style replay gain info
  if (strTagType=="RG_RADIO")
  {
    m_replayGain.iTrackGain = (int)(atof(strTagValue.c_str()) * 100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
  }
  else if (strTagType=="RG_PEAK")
  {
    m_replayGain.fTrackPeak = (float)atof(strTagValue.c_str());
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
  }
  else if (strTagType=="RG_AUDIOPHILE")
  {
    m_replayGain.iAlbumGain = (int)(atof(strTagValue.c_str()) * 100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
  }
  return 0;
}

void CVorbisTag::SplitEntry(const std::string& strTagEntry, std::string& strTagType, std::string& strTagValue)
{
  int nPos = strTagEntry.find( '=' );

  if ( nPos > -1 )
  {
    // we use UTF-8 internally
    strTagValue = strTagEntry.substr( nPos + 1 );
    strTagType = strTagEntry.substr( 0, nPos );
    StringUtils::ToUpper(strTagType);
  }
}
