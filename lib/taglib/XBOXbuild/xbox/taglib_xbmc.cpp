/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// Format-specific parsing moved from xbmc/music/tags/TagLoaderTagLib.cpp.
#include "taglib_xbmc.h"
#include "Metadata.h"
#include "CallbackStream.h"
#include <memory>
#include <cstring>
#include <cstdlib>

#include <id3v1tag.h>
#include <apetag.h>
#include <asftag.h>
#include <id3v1genres.h>
#include <aifffile.h>
#include <apefile.h>
#include <asffile.h>
#include <modfile.h>
#include <mp4file.h>
#include <mpegfile.h>
#include <oggfile.h>
#include <oggflacfile.h>
#include <opusfile.h>
#include <rifffile.h>
#include <speexfile.h>
#include <s3mfile.h>
#include <trueaudiofile.h>
#include <vorbisfile.h>
#include <wavfile.h>
#include <wavpackfile.h>
#include <xmfile.h>
#include <flacfile.h>
#include <itfile.h>
#include <mpcfile.h>
#include <id3v2tag.h>
#include <xiphcomment.h>
#include <mp4tag.h>

#include <textidentificationframe.h>
#include <uniquefileidentifierframe.h>
#include <popularimeterframe.h>
#include <commentsframe.h>
#include <unsynchronizedlyricsframe.h>
#include <attachedpictureframe.h>

#include <tstring.h>
#include <tpropertymap.h>


#if TAGLIB_MAJOR_VERSION != 1 || TAGLIB_MINOR_VERSION < 11
#error The XBMC bridge requires TagLib 1.11 or later in the 1.x series
#endif

using namespace TagLib;
namespace XbmcTagLib
{
template<typename T> bool ParseTag(T* source, Artwork* art, Metadata& tag);

void SetGenre(Metadata &tag, const std::vector<std::string> &values)
{
  /*
   TagLib doesn't resolve ID3v1 genre numbers in the case were only
   a number is specified, thus this workaround.
   */
  std::vector<std::string> genres;
  for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it)
  {
    const std::string& i = *it;
    std::string genre = i;
    if (!genre.empty() && genre.find_first_not_of("0123456789") == std::string::npos)
    {
      int number = strtol(i.c_str(), NULL, 10);
      if (number >= 0 && number < 256)
        genre = ID3v1::genre(number).to8Bit(true);
    }
    genres.push_back(genre);
  }
  tag.Strings(XBMC_TAGLIB_SetGenre, genres);
}

std::vector<std::string> StringListToVectorString(const StringList& stringList)
{
  std::vector<std::string> values;
  for (StringList::ConstIterator it = stringList.begin(); it != stringList.end(); ++it)
    values.push_back(it->to8Bit(true));
  return values;
}

std::vector<std::string> GetASFStringList(const List<ASF::Attribute>& list)
{
  std::vector<std::string> values;
  for (List<ASF::Attribute>::ConstIterator at = list.begin(); at != list.end(); ++at)
    values.push_back(at->toString().to8Bit(true));
  return values;
}

std::vector<std::string> GetID3v2StringList(const ID3v2::FrameList& frameList)
{
  if (frameList.isEmpty()) return std::vector<std::string>();
  const ID3v2::TextIdentificationFrame *frame = dynamic_cast<const ID3v2::TextIdentificationFrame *>(frameList.front());
  if (frame)
    return StringListToVectorString(frame->fieldList());
  return std::vector<std::string>();
}

void SetFlacArt(FLAC::File *flacFile, Artwork *art, Metadata &tag)
{
  FLAC::Picture *cover[2] = {NULL};
  List<FLAC::Picture *> pictures = flacFile->pictureList();
  for (List<FLAC::Picture *>::ConstIterator i = pictures.begin(); i != pictures.end(); ++i)
  {
    FLAC::Picture *picture = *i;
    if (picture->type() == FLAC::Picture::FrontCover)
      cover[0] = picture;
    else // anything else is taken as second priority
      cover[1] = picture;
  }
  for (size_t coverIndex = 0; coverIndex < sizeof(cover) / sizeof(cover[0]); ++coverIndex)
  {
    const FLAC::Picture *c = cover[coverIndex];
    if (c)
    {
      tag.SetCoverArtInfo(c->data().size(), c->mimeType().to8Bit(true));
      if (art)
        art->Set(reinterpret_cast<const unsigned char*>(c->data().data()), c->data().size(), c->mimeType().to8Bit(true));
      return; // one is enough
    }
  }
}
template<>
bool ParseTag(ASF::Tag *asf, Artwork *art, Metadata& tag)
{
  if (!asf)
    return false;

  ReplayGain replayGainInfo;
  tag.SetTitle(asf->title().to8Bit(true));
  const ASF::AttributeListMap& attributeListMap = asf->attributeListMap();
  for (ASF::AttributeListMap::ConstIterator it = attributeListMap.begin(); it != attributeListMap.end(); ++it)
  {
    if (it->second.isEmpty()) continue;
    if (it->first == "Author")
      SetArtist(tag, GetASFStringList(it->second));
    else if (it->first == "WM/ArtistSortOrder")
      SetArtistSort(tag, GetASFStringList(it->second));
    else if (it->first == "WM/AlbumArtist")
      SetAlbumArtist(tag, GetASFStringList(it->second));
    else if (it->first == "WM/AlbumArtistSortOrder")
      SetAlbumArtistSort(tag, GetASFStringList(it->second));
    else if (it->first == "WM/ComposerSortOrder")
      SetComposerSort(tag, GetASFStringList(it->second));
    else if (it->first == "WM/AlbumTitle")
      tag.SetAlbum(it->second.front().toString().to8Bit(true));
    else if (it->first == "WM/TrackNumber" ||
             it->first == "WM/Track")
    {
      if (it->second.front().type() == ASF::Attribute::DWordType)
        tag.SetTrackNumber(it->second.front().toUInt());
      else
        tag.SetTrackNumber(atoi(it->second.front().toString().toCString(true)));
    }
    else if (it->first == "WM/PartOfSet")
      tag.SetDiscNumber(atoi(it->second.front().toString().toCString(true)));
    else if (it->first == "WM/Genre")
      SetGenre(tag, GetASFStringList(it->second));
    else if (it->first == "WM/Mood")
      tag.SetMood(it->second.front().toString().to8Bit(true));
    else if (it->first == "WM/Composer")
      AddArtistRole(tag, "Composer", GetASFStringList(it->second));
    else if (it->first == "WM/Conductor")
      AddArtistRole(tag, "Conductor", GetASFStringList(it->second));
    //No ASF/WMA tag from Taglib for "ensemble"
    else if (it->first == "WM/Writer")
      AddArtistRole(tag, "Lyricist", GetASFStringList(it->second));
    else if (it->first == "WM/ModifiedBy")
      AddArtistRole(tag, "Remixer", GetASFStringList(it->second));
    else if (it->first == "WM/Engineer")
      AddArtistRole(tag, "Engineer", GetASFStringList(it->second));
    else if (it->first == "WM/Producer")
      AddArtistRole(tag, "Producer", GetASFStringList(it->second));
    else if (it->first == "WM/DJMixer")
      AddArtistRole(tag, "DJMixer", GetASFStringList(it->second));
    else if (it->first == "WM/Mixer")
      AddArtistRole(tag, "mixer", GetASFStringList(it->second));
    else if (it->first == "WM/Publisher")
      tag.SetRecordLabel(it->second.front().toString().to8Bit(true));
    else if (it->first == "WM/Script")
    {} // Known unsupported, suppress warnings
    else if (it->first == "WM/Year")
      tag.SetReleaseDate(it->second.front().toString().to8Bit(true));
    else if (it->first == "WM/OriginalReleaseYear")
      tag.SetOriginalDate(it->second.front().toString().to8Bit(true));
    else if (it->first == "WM/SetSubTitle")
      tag.SetDiscSubtitle(it->second.front().toString().to8Bit(true));
    else if (it->first == "MusicBrainz/Artist Id")
      tag.SetMusicBrainzArtistID(GetASFStringList(it->second));
    else if (it->first == "MusicBrainz/Album Id")
      tag.SetMusicBrainzAlbumID(it->second.front().toString().to8Bit(true));
    else if (it->first == "MusicBrainz/Release Group Id")
      tag.SetMusicBrainzReleaseGroupID(it->second.front().toString().to8Bit(true));
    else if (it->first == "MusicBrainz/Album Artist")
      SetAlbumArtist(tag, GetASFStringList(it->second));
    else if (it->first == "MusicBrainz/Album Artist Id")
      tag.SetMusicBrainzAlbumArtistID(GetASFStringList(it->second));
    else if (it->first == "MusicBrainz/Track Id")
      tag.SetMusicBrainzTrackID(it->second.front().toString().to8Bit(true));
    else if (it->first == "MusicBrainz/Album Status")
      tag.SetAlbumReleaseStatus(it->second.front().toString().toCString(true));
    else if (it->first == "MusicBrainz/Album Type")
      SetReleaseType(tag, GetASFStringList(it->second));
    else if (it->first == "MusicIP/PUID")
    {}
    else if (it->first == "WM/BeatsPerMinute")
      tag.SetBPM(atoi(it->second.front().toString().toCString(true)));
    else if (it->first == "replaygain_track_gain" || it->first == "REPLAYGAIN_TRACK_GAIN")
      replayGainInfo.ParseGain(ReplayGain::TRACK, it->second.front().toString().toCString(true));
    else if (it->first == "replaygain_album_gain" || it->first == "REPLAYGAIN_ALBUM_GAIN")
      replayGainInfo.ParseGain(ReplayGain::ALBUM, it->second.front().toString().toCString(true));
    else if (it->first == "replaygain_track_peak" || it->first == "REPLAYGAIN_TRACK_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::TRACK, it->second.front().toString().toCString(true));
    else if (it->first == "replaygain_album_peak" || it->first == "REPLAYGAIN_ALBUM_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::ALBUM, it->second.front().toString().toCString(true));
    else if (it->first == "WM/Picture")
    { // picture
      ASF::Picture pic = it->second.front().toPicture();
      tag.SetCoverArtInfo(pic.picture().size(), pic.mimeType().toCString());
      if (art)
        art->Set(reinterpret_cast<const unsigned char *>(pic.picture().data()), pic.picture().size(), pic.mimeType().toCString());
    }
    else if (tag.Debug())
      tag.Log("unrecognized ASF tag name: %s", it->first.toCString(true));
  }
  // artist may be specified in the ContentDescription block rather than using the 'Author' attribute.
  if (tag.Query(XBMC_TAGLIB_ARTIST_EMPTY) != 0)
    tag.SetArtist(asf->artist().toCString(true));

  if (!asf->comment().isEmpty())
    tag.SetComment(asf->comment().toCString(true));
  tag.SetReplayGain(replayGainInfo);
  tag.SetLoaded(true);
  return true;
}

int POPMtoXBMC(int popm)
{
  // Ratings:
  // FROM: http://www.mediamonkey.com/forum/viewtopic.php?f=7&t=40532&start=30#p391067
  // The following schemes are used by the other POPM-compatible players:
  // WMP/Vista: "Windows Media Player 9 Series" ratings:
  //   1 = 1, 2 = 64, 3=128, 4=196 (not 192), 5=255
  // MediaMonkey (v4.2.1): "no@email" ratings:
  //   0.5=13, 1=1, 1.5=54, 2=64, 2.5=118,
  //   3=128, 3.5=186, 4=196, 4.5=242, 5=255
  //   Note 1 star written as 1 while half a star is 13, a higher value
  // Accommodate these mapped values in a scale from 0-255
  if (popm == 0) return 0;
  if (popm == 1) return 2;
  if (popm < 23) return 1;
  if (popm < 32) return 2;
  if (popm < 64) return 3;
  if (popm < 96) return 4;
  if (popm < 128) return 5;
  if (popm < 160) return 6;
  if (popm < 196) return 7;
  if (popm < 224) return 8;
  if (popm < 255) return 9;
  else return 10;
}

template<>
bool ParseTag(ID3v1::Tag *id3v1, Artwork *art, Metadata& tag)
{
  if (!id3v1) return false;
  tag.SetTitle(id3v1->title().to8Bit(true));
  tag.SetArtist(id3v1->artist().to8Bit(true));
  tag.SetAlbum(id3v1->album().to8Bit(true));
  tag.SetComment(id3v1->comment().to8Bit(true));
  tag.SetGenre(id3v1->genre().to8Bit(true), true);
  tag.SetYear(id3v1->year());
  tag.SetTrackNumber(id3v1->track());
  return true;
}

template<>
bool ParseTag(ID3v2::Tag *id3v2, Artwork *art, Metadata& tag)
{
  if (!id3v2) return false;
  ReplayGain replayGainInfo;

  ID3v2::AttachedPictureFrame *pictures[3] = {NULL};
  const ID3v2::FrameListMap& frameListMap = id3v2->frameListMap();
  for (ID3v2::FrameListMap::ConstIterator it = frameListMap.begin(); it != frameListMap.end(); ++it)
  {
    // It is possible that the taglist is empty. In that case no useable values can be extracted.
    // and we should skip the tag.
    if (it->second.isEmpty()) continue;

    if      (it->first == "TPE1")   SetArtist(tag, GetID3v2StringList(it->second));
    else if (it->first == "TSOP")   SetArtistSort(tag, GetID3v2StringList(it->second));
    else if (it->first == "TALB")   tag.SetAlbum(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TPE2")   SetAlbumArtist(tag, GetID3v2StringList(it->second));
    else if (it->first == "TSO2")   SetAlbumArtistSort(tag, GetID3v2StringList(it->second));
    else if (it->first == "TSOC")   SetComposerSort(tag, GetID3v2StringList(it->second));
    else if (it->first == "TIT2")   tag.SetTitle(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TCON")   SetGenre(tag, GetID3v2StringList(it->second));
    else if (it->first == "TRCK")
      tag.SetTrackNumber(
          static_cast<int>(strtol(it->second.front()->toString().toCString(true), NULL, 10)));
    else if (it->first == "TPOS")
      tag.SetDiscNumber(
          static_cast<int>(strtol(it->second.front()->toString().toCString(true), NULL, 10)));
    else if (it->first == "TDOR" || it->first == "TORY") // TDOR - ID3v2.4, TORY - ID3v2.3
      tag.SetOriginalDate(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TDAT")   {} // empty as taglib has moved the value to TDRC
    else if (it->first == "TCMP")   tag.SetCompilation((strtol(it->second.front()->toString().toCString(true), NULL, 10) == 0) ? false : true);
    else if (it->first == "TENC")   {} // EncodedBy
    else if (it->first == "TCOM")   AddArtistRole(tag, "Composer", GetID3v2StringList(it->second));
    else if (it->first == "TPE3")   AddArtistRole(tag, "Conductor", GetID3v2StringList(it->second));
    else if (it->first == "TEXT")   AddArtistRole(tag, "Lyricist", GetID3v2StringList(it->second));
    else if (it->first == "TPE4")   AddArtistRole(tag, "Remixer", GetID3v2StringList(it->second));
    else if (it->first == "TPUB")   tag.SetRecordLabel(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TCOP")   {} // Copyright message
    else if (it->first == "TDRC")  // taglib concatenates TYER & TDAT into this field if v2.3
      tag.SetReleaseDate(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TDRL")   {} // Not set by Picard or used in community generally
    else if (it->first == "TDTG")   {} // Tagging time
    else if (it->first == "TLAN")   {} // Languages
    else if (it->first == "TMOO")   tag.SetMood(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TSST")
      tag.SetDiscSubtitle(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TBPM")
      tag.SetBPM(
          static_cast<int>(strtol(it->second.front()->toString().toCString(true), NULL, 10)));
    else if (it->first == "USLT")
      // Loop through any lyrics frames. Could there be multiple frames, how to choose?
      for (ID3v2::FrameList::ConstIterator lt = it->second.begin(); lt != it->second.end(); ++lt)
      {
        ID3v2::UnsynchronizedLyricsFrame *lyricsFrame = dynamic_cast<ID3v2::UnsynchronizedLyricsFrame *> (*lt);
        if (lyricsFrame)
          tag.SetLyrics(lyricsFrame->text().to8Bit(true));
      }
    else if (it->first == "COMM")
      // Loop through and look for the main (no description) comment
      for (ID3v2::FrameList::ConstIterator ct = it->second.begin(); ct != it->second.end(); ++ct)
      {
        ID3v2::CommentsFrame *commentsFrame = dynamic_cast<ID3v2::CommentsFrame *> (*ct);
        if (commentsFrame && commentsFrame->description().isEmpty())
          tag.SetComment(commentsFrame->text().to8Bit(true));
      }
    else if (it->first == "TXXX")
      // Loop through and process the UserTextIdentificationFrames
      for (ID3v2::FrameList::ConstIterator ut = it->second.begin(); ut != it->second.end(); ++ut)
      {
        ID3v2::UserTextIdentificationFrame *frame = dynamic_cast<ID3v2::UserTextIdentificationFrame *> (*ut);
        if (!frame) continue;

        // First field is the same as the description
        StringList stringList = frame->fieldList();
        if (stringList.size() <= 1) continue;
        stringList.erase(stringList.begin());
        String desc = frame->description().upper();
        if      (desc == "MUSICBRAINZ ARTIST ID")
          tag.SetMusicBrainzArtistID(StringListToVectorString(stringList));
        else if (desc == "MUSICBRAINZ ALBUM ID")
          tag.SetMusicBrainzAlbumID(stringList.front().to8Bit(true));
        else if (desc == "MUSICBRAINZ RELEASE GROUP ID")
          tag.SetMusicBrainzReleaseGroupID(stringList.front().to8Bit(true));
        else if (desc == "MUSICBRAINZ ALBUM ARTIST ID")
          tag.SetMusicBrainzAlbumArtistID(StringListToVectorString(stringList));
        else if (desc == "MUSICBRAINZ ALBUM ARTIST")
          SetAlbumArtist(tag, StringListToVectorString(stringList));
        else if (desc == "MUSICBRAINZ ALBUM TYPE")
          SetReleaseType(tag, StringListToVectorString(stringList));
        else if (desc == "MUSICBRAINZ ALBUM STATUS")
          tag.SetAlbumReleaseStatus(stringList.front().to8Bit(true));
        else if (desc == "REPLAYGAIN_TRACK_GAIN")
          replayGainInfo.ParseGain(ReplayGain::TRACK, stringList.front().toCString(true));
        else if (desc == "REPLAYGAIN_ALBUM_GAIN")
          replayGainInfo.ParseGain(ReplayGain::ALBUM, stringList.front().toCString(true));
        else if (desc == "REPLAYGAIN_TRACK_PEAK")
          replayGainInfo.ParsePeak(ReplayGain::TRACK, stringList.front().toCString(true));
        else if (desc == "REPLAYGAIN_ALBUM_PEAK")
          replayGainInfo.ParsePeak(ReplayGain::ALBUM, stringList.front().toCString(true));
        else if (desc == "ALBUMARTIST" || desc == "ALBUM ARTIST")
          SetAlbumArtist(tag, StringListToVectorString(stringList));
        else if (desc == "ALBUMARTISTSORT" || desc == "ALBUM ARTIST SORT")
          SetAlbumArtistSort(tag, StringListToVectorString(stringList));
        else if (desc == "ARTISTS")
          SetArtistHints(tag, StringListToVectorString(stringList));
        else if (desc == "ALBUMARTISTS" || desc == "ALBUM ARTISTS")
          SetAlbumArtistHints(tag, StringListToVectorString(stringList));
        else if (desc == "WRITER")  // How Picard >1.3 tags writer in ID3
          AddArtistRole(tag, "Writer", StringListToVectorString(stringList));
        else if (desc == "COMPOSERSORT" || desc == "COMPOSER SORT")
          SetComposerSort(tag, StringListToVectorString(stringList));
        else if (desc == "MOOD")
          tag.SetMood(stringList.front().to8Bit(true));
        else if (tag.Debug())
          tag.Log("unrecognized user text tag detected: TXXX:%s",
                    frame->description().toCString(true));
      }
    else if (it->first == "TIPL")
      // Loop through and process the involved people list
      // For example Arranger, Engineer, Producer, DJMixer or Mixer
      // In fieldlist every odd field is a function, and every even is an artist or a comma delimited list of artists.
      for (ID3v2::FrameList::ConstIterator ip = it->second.begin(); ip != it->second.end(); ++ip)
      {
        ID3v2::TextIdentificationFrame *tiplframe = dynamic_cast<ID3v2::TextIdentificationFrame*> (*ip);
        if (tiplframe)
          AddArtistRole(tag, StringListToVectorString(tiplframe->fieldList()));
      }
    else if (it->first == "TMCL")
      // Loop through and process the musician credits list
      // It is a mapping between the instrument and the person that played it, but also includes "orchestra" or "soloist".
      // In fieldlist every odd field is an instrument, and every even is an artist or a comma delimited list of artists.
      for (ID3v2::FrameList::ConstIterator ip = it->second.begin(); ip != it->second.end(); ++ip)
      {
        ID3v2::TextIdentificationFrame *tiplframe = dynamic_cast<ID3v2::TextIdentificationFrame*> (*ip);
        if (tiplframe)
          AddArtistRole(tag, StringListToVectorString(tiplframe->fieldList()));
      }
    else if (it->first == "UFID")
      // Loop through any UFID frames and set them
      for (ID3v2::FrameList::ConstIterator ut = it->second.begin(); ut != it->second.end(); ++ut)
      {
        ID3v2::UniqueFileIdentifierFrame *ufid = dynamic_cast<ID3v2::UniqueFileIdentifierFrame*> (*ut);
        if (ufid && ufid->owner() == "http://musicbrainz.org")
        {
          // MusicBrainz pads with a \0, but the spec requires binary, be cautious
          char cUfid[64];
          int max_size = std::min(static_cast<int>(ufid->identifier().size()), 63);
          strncpy(cUfid, ufid->identifier().data(), max_size);
          cUfid[max_size] = '\0';
          tag.SetMusicBrainzTrackID(cUfid);
        }
      }
    else if (it->first == "APIC")
      // Loop through all pictures and store the frame pointers for the picture types we want
      for (ID3v2::FrameList::ConstIterator pi = it->second.begin(); pi != it->second.end(); ++pi)
      {
        ID3v2::AttachedPictureFrame *pictureFrame = dynamic_cast<ID3v2::AttachedPictureFrame *> (*pi);
        if (!pictureFrame) continue;

        if      (pictureFrame->type() == ID3v2::AttachedPictureFrame::FrontCover) pictures[0] = pictureFrame;
        else if (pictureFrame->type() == ID3v2::AttachedPictureFrame::Other)      pictures[1] = pictureFrame;
        else if (pi == it->second.begin())                                        pictures[2] = pictureFrame;
      }
    else if (it->first == "POPM")
      // Loop through and process ratings
      for (ID3v2::FrameList::ConstIterator ct = it->second.begin(); ct != it->second.end(); ++ct)
      {
        ID3v2::PopularimeterFrame *popFrame = dynamic_cast<ID3v2::PopularimeterFrame *> (*ct);
        if (!popFrame) continue;

        // @xbmc.org ratings trump others (of course)
        if      (popFrame->email() == "ratings@xbmc.org")
          tag.SetUserrating(popFrame->rating() / 51); //! @todo wtf? Why 51 find some explanation, somewhere...
        else if (tag.Query(XBMC_TAGLIB_USER_RATING) == 0)
        {
          if (popFrame->email() != "Windows Media Player 9 Series" &&
              popFrame->email() != "Banshee" &&
              popFrame->email() != "no@email" &&
              popFrame->email() != "quodlibet@lists.sacredchao.net" &&
              popFrame->email() != "rating@winamp.com")
            tag.Log("unrecognized ratings schema detected: %s",
                      popFrame->email().toCString(true));
          tag.SetUserrating(POPMtoXBMC(popFrame->rating()));
        }
      }
    else if (tag.Debug())
      tag.Log("unrecognized ID3 frame detected: %c%c%c%c", it->first[0], it->first[1],
                it->first[2], it->first[3]);
  } // for

  // Process the extracted picture frames; 0 = CoverArt, 1 = Other, 2 = First Found picture
  for (size_t pictureIndex = 0; pictureIndex < sizeof(pictures) / sizeof(pictures[0]); ++pictureIndex)
  {
    const ID3v2::AttachedPictureFrame *picture = pictures[pictureIndex];
    if (picture)
    {
      std::string  mime =            picture->mimeType().to8Bit(true);
#if (TAGLIB_MAJOR_VERSION >= 2)
      unsigned int size =            picture->picture().size();
#else
      TagLib::uint size =            picture->picture().size();
#endif
      tag.SetCoverArtInfo(size, mime);
      if (art)
        art->Set(reinterpret_cast<const unsigned char*>(picture->picture().data()), size, mime);

      // Stop after we find the first picture for now.
      break;
    }
  }


  if (!id3v2->comment().isEmpty())
    tag.SetComment(id3v2->comment().toCString(true));

  tag.SetReplayGain(replayGainInfo);
  return true;
}

template<>
bool ParseTag(APE::Tag *ape, Artwork *art, Metadata& tag)
{
  if (!ape)
    return false;

  ReplayGain replayGainInfo;
  const APE::ItemListMap itemListMap = ape->itemListMap();
  for (APE::ItemListMap::ConstIterator it = itemListMap.begin(); it != itemListMap.end(); ++it)
  {
    if (it->first == "ARTIST")
      SetArtist(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "ARTISTSORT")
      SetArtistSort(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "ARTISTS")
      SetArtistHints(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "ALBUMARTIST" || it->first == "ALBUM ARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "ALBUMARTISTSORT")
      SetAlbumArtistSort(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "ALBUMARTISTS" || it->first == "ALBUM ARTISTS")
      SetAlbumArtistHints(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "COMPOSERSORT")
      SetComposerSort(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "ALBUM")
      tag.SetAlbum(it->second.toString().to8Bit(true));
    else if (it->first == "TITLE")
      tag.SetTitle(it->second.toString().to8Bit(true));
    else if (it->first == "TRACKNUMBER" || it->first == "TRACK")
      tag.SetTrackNumber(it->second.toString().toInt());
    else if (it->first == "DISCNUMBER" || it->first == "DISC")
      tag.SetDiscNumber(it->second.toString().toInt());
    else if (it->first == "YEAR")
      tag.SetReleaseDate(it->second.toString().to8Bit(true));
    else if (it->first == "DISCSUBTITLE")
      tag.SetDiscSubtitle(it->second.toString().to8Bit(true));
    else if (it->first == "ORIGINALYEAR")
      tag.SetOriginalDate(it->second.toString().to8Bit(true));
    else if (it->first == "GENRE")
      SetGenre(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "MOOD")
      tag.SetMood(it->second.toString().to8Bit(true));
    else if (it->first == "COMMENT")
      tag.SetComment(it->second.toString().to8Bit(true));
    else if (it->first == "CUESHEET")
      tag.SetCueSheet(it->second.toString().to8Bit(true));
    else if (it->first == "ENCODEDBY")
    {}
    else if (it->first == "COMPOSER")
      AddArtistRole(tag, "Composer", StringListToVectorString(it->second.values()));
    else if (it->first == "CONDUCTOR")
      AddArtistRole(tag, "Conductor", StringListToVectorString(it->second.values()));
    else if (it->first == "BAND")
      AddArtistRole(tag, "Band", StringListToVectorString(it->second.values()));
    else if (it->first == "ENSEMBLE")
      AddArtistRole(tag, "Ensemble", StringListToVectorString(it->second.values()));
    else if (it->first == "LYRICIST")
      AddArtistRole(tag, "Lyricist", StringListToVectorString(it->second.values()));
    else if (it->first == "WRITER")
      AddArtistRole(tag, "Writer", StringListToVectorString(it->second.values()));
    else if ((it->first == "MIXARTIST") || (it->first == "REMIXER"))
      AddArtistRole(tag, "Remixer", StringListToVectorString(it->second.values()));
    else if (it->first == "ARRANGER")
      AddArtistRole(tag, "Arranger", StringListToVectorString(it->second.values()));
    else if (it->first == "ENGINEER")
      AddArtistRole(tag, "Engineer", StringListToVectorString(it->second.values()));
    else if (it->first == "PRODUCER")
      AddArtistRole(tag, "Producer", StringListToVectorString(it->second.values()));
    else if (it->first == "DJMIXER")
      AddArtistRole(tag, "DJMixer", StringListToVectorString(it->second.values()));
    else if (it->first == "MIXER")
      AddArtistRole(tag, "Mixer", StringListToVectorString(it->second.values()));
    else if (it->first == "PERFORMER")
      // Picard uses PERFORMER tag as musician credits list formatted "name (instrument)"
      AddArtistInstrument(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "LABEL")
      tag.SetRecordLabel(it->second.toString().to8Bit(true));
    else if (it->first == "COMPILATION")
      tag.SetCompilation(it->second.toString().toInt() == 1);
    else if (it->first == "LYRICS")
      tag.SetLyrics(it->second.toString().to8Bit(true));
    else if (it->first == "REPLAYGAIN_TRACK_GAIN")
      replayGainInfo.ParseGain(ReplayGain::TRACK, it->second.toString().toCString(true));
    else if (it->first == "REPLAYGAIN_ALBUM_GAIN")
      replayGainInfo.ParseGain(ReplayGain::ALBUM, it->second.toString().toCString(true));
    else if (it->first == "REPLAYGAIN_TRACK_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::TRACK, it->second.toString().toCString(true));
    else if (it->first == "REPLAYGAIN_ALBUM_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::ALBUM, it->second.toString().toCString(true));
    else if (it->first == "MUSICBRAINZ_ARTISTID")
      tag.SetMusicBrainzArtistID(StringListToVectorString(it->second.values()));
    else if (it->first == "MUSICBRAINZ_ALBUMARTISTID")
      tag.SetMusicBrainzAlbumArtistID(StringListToVectorString(it->second.values()));
    else if (it->first == "MUSICBRAINZ_ALBUMARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "MUSICBRAINZ_ALBUMID")
      tag.SetMusicBrainzAlbumID(it->second.toString().to8Bit(true));
    else if (it->first == "MUSICBRAINZ_RELEASEGROUPID")
      tag.SetMusicBrainzReleaseGroupID(it->second.toString().to8Bit(true));
    else if (it->first == "MUSICBRAINZ_TRACKID")
      tag.SetMusicBrainzTrackID(it->second.toString().to8Bit(true));
    else if (it->first == "MUSICBRAINZ_ALBUMTYPE")
      SetReleaseType(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "BPM")
      tag.SetBPM(it->second.toString().toInt());
    else if (it->first == "MUSICBRAINZ_ALBUMSTATUS")
      tag.SetAlbumReleaseStatus(it->second.toString().to8Bit(true));
    else if (it->first == "COVER ART (FRONT)")
    {
      TagLib::ByteVector tdata = it->second.binaryData();
      // The image data follows a null byte, which can optionally be preceded by a filename
      const uint offset = tdata.find('\0') + 1;
      ByteVector bv(tdata.data() + offset, tdata.size() - offset);
      // Infer the mimetype
      std::string mime;
      if (bv.startsWith("\xFF\xD8\xFF"))
        mime = "image/jpeg";
      else if (bv.startsWith("\x89\x50\x4E\x47"))
        mime = "image/png";
      else if (bv.startsWith("\x47\x49\x46\x38"))
        mime = "image/gif";
      else if (bv.startsWith("\x42\x4D"))
        mime = "image/bmp";
      if ((offset > 0) && (offset <= tdata.size()) && (mime.size() > 0))
      {
        tag.SetCoverArtInfo(bv.size(), mime);
        if (art)
          art->Set(reinterpret_cast<const unsigned char*>(bv.data()), bv.size(), mime);
      }
    }
    else if (tag.Debug())
      tag.Log("unrecognized APE tag: %s", it->first.toCString(true));
  }

  tag.SetReplayGain(replayGainInfo);
  return true;
}

template<>
bool ParseTag(Ogg::XiphComment *xiph, Artwork *art, Metadata& tag)
{
  if (!xiph)
    return false;

  ReplayGain replayGainInfo;

  const Ogg::FieldListMap& fieldListMap = xiph->fieldListMap();
  for (Ogg::FieldListMap::ConstIterator it = fieldListMap.begin(); it != fieldListMap.end(); ++it)
  {
    if (it->second.isEmpty()) continue;
    if (it->first == "ARTIST")
      SetArtist(tag, StringListToVectorString(it->second));
    else if (it->first == "ARTISTSORT")
      SetArtistSort(tag, StringListToVectorString(it->second));
    else if (it->first == "ARTISTS")
      SetArtistHints(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUMARTIST" || it->first == "ALBUM ARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUMARTISTSORT" || it->first == "ALBUM ARTIST SORT")
      SetAlbumArtistSort(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUMARTISTS" || it->first == "ALBUM ARTISTS")
      SetAlbumArtistHints(tag, StringListToVectorString(it->second));
    else if (it->first == "COMPOSERSORT")
      SetComposerSort(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUM")
      tag.SetAlbum(it->second.front().to8Bit(true));
    else if (it->first == "TITLE")
      tag.SetTitle(it->second.front().to8Bit(true));
    else if (it->first == "TRACKNUMBER")
      tag.SetTrackNumber(it->second.front().toInt());
    else if (it->first == "DISCNUMBER")
      tag.SetDiscNumber(it->second.front().toInt());
    else if (it->first == "YEAR" || it->first == "DATE")
      tag.AddReleaseDate(it->second.front().to8Bit(true));
    else if (it->first == "GENRE")
      SetGenre(tag, StringListToVectorString(it->second));
    else if (it->first == "MOOD")
      tag.SetMood(it->second.front().to8Bit(true));
    else if (it->first == "COMMENT")
      tag.SetComment(it->second.front().to8Bit(true));
    else if (it->first == "ORIGINALYEAR" || it->first == "ORIGINALDATE")
      tag.AddOriginalDate(it->second.front().to8Bit(true));
    else if (it->first == "CUESHEET")
      tag.SetCueSheet(it->second.front().to8Bit(true));
    else if (it->first == "DISCSUBTITLE")
      tag.SetDiscSubtitle(it->second.front().to8Bit(true));
    else if (it->first == "ENCODEDBY")
    {} // Known but unsupported, suppress warnings
    else if (it->first == "COMPOSER")
      AddArtistRole(tag, "Composer", StringListToVectorString(it->second));
    else if (it->first == "CONDUCTOR")
      AddArtistRole(tag, "Conductor", StringListToVectorString(it->second));
    else if (it->first == "BAND")
      AddArtistRole(tag, "Band", StringListToVectorString(it->second));
    else if (it->first == "ENSEMBLE")
      AddArtistRole(tag, "Ensemble", StringListToVectorString(it->second));
    else if (it->first == "LYRICIST")
      AddArtistRole(tag, "Lyricist", StringListToVectorString(it->second));
    else if (it->first == "WRITER")
      AddArtistRole(tag, "Writer", StringListToVectorString(it->second));
    else if ((it->first == "MIXARTIST") || (it->first == "REMIXER"))
      AddArtistRole(tag, "Remixer", StringListToVectorString(it->second));
    else if (it->first == "ARRANGER")
      AddArtistRole(tag, "Arranger", StringListToVectorString(it->second));
    else if (it->first == "ENGINEER")
      AddArtistRole(tag, "Engineer", StringListToVectorString(it->second));
    else if (it->first == "PRODUCER")
      AddArtistRole(tag, "Producer", StringListToVectorString(it->second));
    else if (it->first == "DJMIXER")
      AddArtistRole(tag, "DJMixer", StringListToVectorString(it->second));
    else if (it->first == "MIXER")
      AddArtistRole(tag, "Mixer", StringListToVectorString(it->second));
    else if (it->first == "PERFORMER")
      // Picard uses PERFORMER tag as musician credits list formatted "name (instrument)"
      AddArtistInstrument(tag, StringListToVectorString(it->second));
    else if (it->first == "LABEL")
      tag.SetRecordLabel(it->second.front().to8Bit(true));
    else if (it->first == "COMPILATION")
      tag.SetCompilation(it->second.front().toInt() == 1);
    else if (it->first == "LYRICS")
      tag.SetLyrics(it->second.front().to8Bit(true));
    else if (it->first == "REPLAYGAIN_TRACK_GAIN")
      replayGainInfo.ParseGain(ReplayGain::TRACK, it->second.front().toCString(true));
    else if (it->first == "REPLAYGAIN_ALBUM_GAIN")
      replayGainInfo.ParseGain(ReplayGain::ALBUM, it->second.front().toCString(true));
    else if (it->first == "REPLAYGAIN_TRACK_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::TRACK, it->second.front().toCString(true));
    else if (it->first == "REPLAYGAIN_ALBUM_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::ALBUM, it->second.front().toCString(true));
    else if (it->first == "MUSICBRAINZ_ARTISTID")
      tag.SetMusicBrainzArtistID(StringListToVectorString(it->second));
    else if (it->first == "MUSICBRAINZ_ALBUMARTISTID")
      tag.SetMusicBrainzAlbumArtistID(StringListToVectorString(it->second));
    else if (it->first == "MUSICBRAINZ_ALBUMARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second));
    else if (it->first == "MUSICBRAINZ_ALBUMID")
      tag.SetMusicBrainzAlbumID(it->second.front().to8Bit(true));
    else if (it->first == "MUSICBRAINZ_RELEASEGROUPID")
      tag.SetMusicBrainzReleaseGroupID(it->second.front().to8Bit(true));
    else if (it->first == "MUSICBRAINZ_TRACKID")
      tag.SetMusicBrainzTrackID(it->second.front().to8Bit(true));
    else if (it->first == "RELEASETYPE")
      SetReleaseType(tag, StringListToVectorString(it->second));
    else if (it->first == "BPM")
      tag.SetBPM(strtol(it->second.front().toCString(true), NULL, 10));
    else if (it->first == "RELEASESTATUS")
      tag.SetAlbumReleaseStatus(it->second.front().toCString(true));
    else if (it->first == "RATING")
    {
      // Vorbis ratings are a mess because the standard forgot to mention anything about them.
      // If you want to see how emotive the issue is and the varying standards, check here:
      // http://forums.winamp.com/showthread.php?t=324512
      // The most common standard in that thread seems to be a 0-100 scale for 1-5 stars.
      // So, that's what we'll support for now.
      int iUserrating = it->second.front().toInt();
      if (iUserrating > 0 && iUserrating <= 100)
        tag.SetUserrating((iUserrating / 10));
    }
    else if (tag.Debug())
      tag.Log("unrecognized XipComment name: %s", it->first.toCString(true));
  }

  List<FLAC::Picture *> pictureList = xiph->pictureList();
  FLAC::Picture *cover[2] = {NULL};

  for (List<FLAC::Picture *>::ConstIterator it = pictureList.begin(); it != pictureList.end(); ++it)
  {
    FLAC::Picture *picture = *it;
    if (picture->type() == FLAC::Picture::FrontCover)
      cover[0] = picture;
    else // anything else is taken as second priority
      cover[1] = picture;
  }
  for (size_t coverIndex = 0; coverIndex < sizeof(cover) / sizeof(cover[0]); ++coverIndex)
  {
    const FLAC::Picture *c = cover[coverIndex];
    if (c)
    {
      tag.SetCoverArtInfo(c->data().size(), c->mimeType().to8Bit(true));
      if (art)
        art->Set(reinterpret_cast<const unsigned char*>(c->data().data()), c->data().size(), c->mimeType().to8Bit(true));
      break; // one is enough
    }
  }

  if (!xiph->comment().isEmpty())
    tag.SetComment(xiph->comment().toCString(true));

  tag.SetReplayGain(replayGainInfo);
  return true;
}

template<>
bool ParseTag(MP4::Tag *mp4, Artwork *art, Metadata& tag)
{
  if (!mp4)
    return false;

  ReplayGain replayGainInfo;
  const MP4::ItemMap itemMap = mp4->itemMap();
  for (MP4::ItemMap::ConstIterator it = itemMap.begin(); it != itemMap.end(); ++it)
  {
    if (it->first != "cpil" && it->first != "trkn" && it->first != "disk" &&
        it->first != "tmpo" && it->first != "covr" && it->second.toStringList().isEmpty())
      continue;
    if (it->first == "\251nam")
      tag.SetTitle(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "\251ART")
      SetArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "soar")
      SetArtistSort(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:ARTISTS")
      SetArtistHints(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "\251alb")
      tag.SetAlbum(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "aART")
      SetAlbumArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "soaa")
      SetAlbumArtistSort(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:albumartists" ||
             it->first == "----:com.apple.iTunes:ALBUMARTISTS")
      SetAlbumArtistHints(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "soco")
      SetComposerSort(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "\251gen")
      SetGenre(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:MOOD")
      tag.SetMood(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "\251cmt")
      tag.SetComment(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "\251wrt")
      AddArtistRole(tag, "Composer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:CONDUCTOR")
      AddArtistRole(tag, "Conductor", StringListToVectorString(it->second.toStringList()));
    //No MP4 standard tag for "ensemble"
    else if (it->first == "----:com.apple.iTunes:LYRICIST")
      AddArtistRole(tag, "Lyricist", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:REMIXER")
      AddArtistRole(tag, "Remixer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:ENGINEER")
      AddArtistRole(tag, "Engineer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:PRODUCER")
      AddArtistRole(tag, "Producer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:DJMIXER")
      AddArtistRole(tag, "DJMixer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:MIXER")
      AddArtistRole(tag, "Mixer", StringListToVectorString(it->second.toStringList()));
    //No MP4 standard tag for musician credits
    else if (it->first == "----:com.apple.iTunes:LABEL")
      tag.SetRecordLabel(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "----:com.apple.iTunes:DISCSUBTITLE")
      tag.SetDiscSubtitle(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "cpil")
      tag.SetCompilation(it->second.toBool());
    else if (it->first == "trkn")
      tag.SetTrackNumber(it->second.toIntPair().first);
    else if (it->first == "disk")
      tag.SetDiscNumber(it->second.toIntPair().first);
    else if (it->first == "\251day")
      tag.SetReleaseDate(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "----:com.apple.iTunes:originaldate")
      tag.SetOriginalDate(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "----:com.apple.iTunes:replaygain_track_gain" ||
             it->first == "----:com.apple.iTunes:REPLAYGAIN_TRACK_GAIN")
      replayGainInfo.ParseGain(ReplayGain::TRACK, it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:replaygain_album_gain" ||
             it->first == "----:com.apple.iTunes:REPLAYGAIN_ALBUM_GAIN")
      replayGainInfo.ParseGain(ReplayGain::ALBUM, it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:replaygain_track_peak" ||
             it->first == "----:com.apple.iTunes:REPLAYGAIN_TRACK_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::TRACK, it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:replaygain_album_peak" ||
             it->first == "----:com.apple.iTunes:REPLAYGAIN_ALBUM_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::ALBUM, it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Artist Id")
      tag.SetMusicBrainzArtistID(StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Artist Id")
      tag.SetMusicBrainzAlbumArtistID(StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Artist")
      SetAlbumArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Id")
      tag.SetMusicBrainzAlbumID(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Release Group Id")
      tag.SetMusicBrainzReleaseGroupID(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Track Id")
      tag.SetMusicBrainzTrackID(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Type")
      SetReleaseType(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Status")
      tag.SetAlbumReleaseStatus(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "tmpo")
      tag.SetBPM(it->second.toIntPair().first);
    else if (it->first == "covr")
    {
      MP4::CoverArtList coverArtList = it->second.toCoverArtList();
      for (MP4::CoverArtList::ConstIterator pt = coverArtList.begin(); pt != coverArtList.end(); ++pt)
      {
        std::string mime;
        switch (pt->format())
        {
          case MP4::CoverArt::PNG:
            mime = "image/png";
            break;
          case MP4::CoverArt::JPEG:
            mime = "image/jpeg";
            break;
          default:
            break;
        }
        if (mime.empty())
          continue;
        tag.SetCoverArtInfo(pt->data().size(), mime);
        if (art)
          art->Set(reinterpret_cast<const unsigned char *>(pt->data().data()), pt->data().size(), mime);
        break; // one is enough
      }
    }
  }

  if (!mp4->comment().isEmpty())
    tag.SetComment(mp4->comment().toCString(true));

  tag.SetReplayGain(replayGainInfo);
  return true;
}

template<>
bool ParseTag(Tag *genericTag, Artwork *art, Metadata& tag)
{
  if (!genericTag)
    return false;

  PropertyMap properties = genericTag->properties();
  for (PropertyMap::ConstIterator it = properties.begin(); it != properties.end(); ++it)
  {
    if (it->second.isEmpty()) continue;
    if (it->first == "ARTIST")
      SetArtist(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUM")
      tag.SetAlbum(it->second.front().to8Bit(true));
    else if (it->first == "TITLE")
      tag.SetTitle(it->second.front().to8Bit(true));
    else if (it->first == "TRACKNUMBER")
      tag.SetTrackNumber(it->second.front().toInt());
    else if (it->first == "YEAR")
      tag.SetYear(it->second.front().toInt());
    else if (it->first == "GENRE")
      SetGenre(tag, StringListToVectorString(it->second));
    else if (it->first == "COMMENT")
      tag.SetComment(it->second.front().to8Bit(true));
  }

  return true;
}





bool Read(const XbmcTagLibRequest& request)
{
  CallbackStream input(request);
  IOStream* stream = &input;
  const std::string strExtension(request.extension);
  Metadata tag(request);
  Artwork artwork(tag);
  Artwork* art = request.read_art ? &artwork : NULL;
  TagLib::File*              file = NULL;
  TagLib::APE::File*         apeFile = NULL;
  TagLib::ASF::File*         asfFile = NULL;
  TagLib::FLAC::File*        flacFile = NULL;
  TagLib::MP4::File*         mp4File = NULL;
  TagLib::MPC::File*         mpcFile = NULL;
  TagLib::MPEG::File*        mpegFile = NULL;
  TagLib::Ogg::Vorbis::File* oggVorbisFile = NULL;
  TagLib::Ogg::FLAC::File*   oggFlacFile = NULL;
  TagLib::Ogg::Opus::File*   oggOpusFile = NULL;
  TagLib::TrueAudio::File*   ttaFile = NULL;
  TagLib::WavPack::File*     wvFile = NULL;
  TagLib::RIFF::WAV::File *  wavFile = NULL;
  TagLib::RIFF::AIFF::File * aiffFile = NULL;



  if (strExtension == "ape")
    file = apeFile = new APE::File(stream);
  else if (strExtension == "asf" || strExtension == "wmv" || strExtension == "wma")
    file = asfFile = new ASF::File(stream);
  else if (strExtension == "flac")
    file = flacFile = new FLAC::File(stream, ID3v2::FrameFactory::instance());
  else if (strExtension == "it")
    file = new IT::File(stream);
  else if (strExtension == "mod" || strExtension == "module" || strExtension == "nst" || strExtension == "wow")
    file = new Mod::File(stream);
  else if (strExtension == "mp4" || strExtension == "m4a" || strExtension == "m4v" ||
           strExtension == "m4r" || strExtension == "m4b" ||
           strExtension == "m4p" || strExtension == "3g2")
    file = mp4File = new MP4::File(stream);
  else if (strExtension == "mpc")
    file = mpcFile = new MPC::File(stream);
  else if (strExtension == "mp3" || strExtension == "aac")
    file = mpegFile = new MPEG::File(stream, ID3v2::FrameFactory::instance());
  else if (strExtension == "s3m")
    file = new S3M::File(stream);
  else if (strExtension == "tta")
    file = ttaFile = new TrueAudio::File(stream, ID3v2::FrameFactory::instance());
  else if (strExtension == "wv")
    file = wvFile = new WavPack::File(stream);
  else if (strExtension == "aif" || strExtension == "aiff")
    file = aiffFile = new RIFF::AIFF::File(stream);
  else if (strExtension == "wav")
    file = wavFile = new RIFF::WAV::File(stream);
  else if (strExtension == "xm")
    file = new XM::File(stream);
  else if (strExtension == "ogg")
    file = oggVorbisFile = new Ogg::Vorbis::File(stream);
  else if (strExtension == "opus")
    file = oggOpusFile = new Ogg::Opus::File(stream);
  else if (strExtension == "oga") // Leave this madness until last - oga container can have Vorbis or FLAC
  {
    file = oggFlacFile = new Ogg::FLAC::File(stream);
    if (!file || !file->isValid())
    {
      delete file;
      file = NULL;
      oggFlacFile = NULL;
      stream->seek(0);
      file = oggVorbisFile = new Ogg::Vorbis::File(stream);
    }
  }
  std::auto_ptr<TagLib::File> owner(file);
  if (!file || !file->isOpen() || !file->isValid())
    throw std::runtime_error("unsupported or invalid audio file");

  APE::Tag *ape = NULL;
  ASF::Tag *asf = NULL;
  MP4::Tag *mp4 = NULL;
  ID3v1::Tag *id3v1 = NULL;
  ID3v2::Tag *id3v2 = NULL;
  Ogg::XiphComment *xiph = NULL;
  Tag *genericTag = NULL;

  if (apeFile)
    ape = apeFile->APETag(false);
  else if (asfFile)
    asf = asfFile->tag();
  else if (flacFile)
  {
    xiph = flacFile->xiphComment(false);
    id3v2 = flacFile->ID3v2Tag(false);
  }
  else if (mp4File)
    mp4 = mp4File->tag();
  else if (mpegFile)
  {
    id3v1 = mpegFile->ID3v1Tag(false);
    id3v2 = mpegFile->ID3v2Tag(false);
    ape = mpegFile->APETag(false);
  }
  else if (oggFlacFile)
    xiph = oggFlacFile->tag();
  else if (oggVorbisFile)
    xiph = oggVorbisFile->tag();
  else if (oggOpusFile)
    xiph = oggOpusFile->tag();
  else if (ttaFile)
    id3v2 = ttaFile->ID3v2Tag(false);
  else if (aiffFile)
    id3v2 = aiffFile->tag();
  else if (wavFile)
    id3v2 = wavFile->ID3v2Tag();
  else if (wvFile)
    ape = wvFile->APETag(false);
  else if (mpcFile)
    ape = mpcFile->APETag(false);
  else    // This is a catch all to get generic information for other files types (s3m, xm, it, mod, etc)
    genericTag = file->tag();

  if (file->audioProperties())
  {
    tag.SetDuration(file->audioProperties()->length());
    tag.SetBitRate(file->audioProperties()->bitrate());
    tag.SetNoOfChannels(file->audioProperties()->channels());
    tag.SetSampleRate(file->audioProperties()->sampleRate());
  }

  if (asf)
    ParseTag(asf, art, tag);
  if (id3v1)
    ParseTag(id3v1, art, tag);
  if (id3v2)
    ParseTag(id3v2, art, tag);
  if (genericTag)
    ParseTag(genericTag, art, tag);
  if (mp4)
    ParseTag(mp4, art, tag);
  if (xiph) // xiph tags override id3v2 tags in badly tagged FLACs
    ParseTag(xiph, art, tag);
  if (ape && (!id3v2 || request.prefer_ape)) // ape tags override id3v2 if we're prioritising them
    ParseTag(ape, art, tag);

  // art for flac files is outside the tag
  if (flacFile)
    SetFlacArt(flacFile, art, tag);

  return true;
}
} // namespace XbmcTagLib

extern "C" unsigned int XBMC_TAGLIB_CALL xbmc_taglib_version(void)
{
  return XBMC_TAGLIB_ABI_VERSION;
}

extern "C" int XBMC_TAGLIB_CALL xbmc_taglib_read(
    const XbmcTagLibRequest* request, char* error, unsigned int error_size)
{
  if (error && error_size) error[0] = '\0';
  try
  {
    if (!request || request->size != sizeof(XbmcTagLibRequest) ||
        request->abi_version != XBMC_TAGLIB_ABI_VERSION ||
        !request->filename || !request->extension ||
        !request->io.read || !request->io.seek || !request->io.length ||
        !request->callbacks.event || !request->callbacks.query)
      throw std::runtime_error("invalid TagLib bridge request or ABI version");
    return XbmcTagLib::Read(*request) ? 1 : 0;
  }
  catch (const std::exception& ex)
  {
    if (error && error_size)
    {
      std::strncpy(error, ex.what(), error_size - 1);
      error[error_size - 1] = '\0';
    }
  }
  catch (...)
  {
    if (error && error_size)
    {
      std::strncpy(error, "unknown TagLib exception", error_size - 1);
      error[error_size - 1] = '\0';
    }
  }
  return 0;
}
