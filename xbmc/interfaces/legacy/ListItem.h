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

#pragma once

#include <map>
#include <vector>

#include "cores/playercorefactory/PlayerCoreFactory.h"

#include "AddonClass.h"
#include "Tuple.h"
#include "Dictionary.h"
#include "Alternative.h"
#include "CallbackHandler.h"
#include "ListItem.h"
#include "music/tags/MusicInfoTag.h"
#include "FileItem.h"
#include "AddonString.h"
#include "Tuple.h"
#include "commons/Exception.h"
#include "InfoTagVideo.h"
#include "InfoTagMusic.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    XBMCCOMMONS_STANDARD_EXCEPTION(ListItemException);

    // This is a type that represents either a String or a String Tuple
    typedef Alternative<StringOrInt,Tuple<String, StringOrInt> > InfoLabelStringOrTuple;

    // This type is either a String or a list of InfoLabelStringOrTuple types
    typedef Alternative<StringOrInt, std::vector<InfoLabelStringOrTuple> > InfoLabelValue;

    // The type contains the dictionary values for the ListItem::setInfo call. 
    // The values in the dictionary can be either a String, or a list of items.
    // If it's a list of items then the items can be either a String or a Tuple.
    typedef Dictionary<InfoLabelValue> InfoLabelDict;

    class ListItem : public AddonClass
    {
    public:
#ifndef SWIG
      CFileItemPtr item;
      bool m_offscreen;
#endif

      ListItem(const String& label = emptyString, 
               const String& label2 = emptyString,
               const String& iconImage = emptyString,
               const String& thumbnailImage = emptyString,
               const String& path = emptyString,
               bool offscreen = false);

#ifndef SWIG
      inline ListItem(CFileItemPtr pitem) : item(pitem) {}

      static inline AddonClass::Ref<ListItem> fromString(const String& str) 
      { 
        AddonClass::Ref<ListItem> ret = AddonClass::Ref<ListItem>(new ListItem());
        ret->item.reset(new CFileItem(str));
        return ret;
      }
#endif

      virtual ~ListItem();

      /**
       * getLabel() -- Returns the listitem label.\n
       * \n
       * example:
       *   - label = self.list.getSelectedItem().getLabel()
       */
      String getLabel();

      /**
       * getLabel2() -- Returns the listitem label.\n
       * \n
       * example:
       *   - label = self.list.getSelectedItem().getLabel2()
       */
      String getLabel2();

      /**
       * setLabel(label) -- Sets the listitem's label.\n
       * \n
       * label          : string or unicode - text string.\n
       * \n
       * example:
       *   - self.list.getSelectedItem().setLabel('Casino Royale')
       */
      void setLabel(const String& label);

      /**
       * setLabel2(label) -- Sets the listitem's label2.\n
       * \n
       * label          : string or unicode - text string.\n
       * \n
       * example:
       *   - self.list.getSelectedItem().setLabel2('Casino Royale')
       */
      void setLabel2(const String& label);

      /**
       * setIconImage(icon) -- Sets the listitem's icon image.\n
       * \n
       * icon            : string - image filename.\n
       * \n
       * example:
       *   - self.list.getSelectedItem().setIconImage('emailread.png')
       */
      void setIconImage(const String& iconImage);

      /**
       * setThumbnailImage(thumbFilename) -- Sets the listitem's thumbnail image.\n
       * \n
       * thumb           : string - image filename.\n
       * \n
       * example:
       *   - self.list.getSelectedItem().setThumbnailImage('emailread.png')
       */
      void setThumbnailImage(const String& thumbFilename);

      /**
       * setArt(values) -- Sets the listitem's art
       * \n
       * values              : dictionary - pairs of { label: value }.\n
       *
       * - Some default art values (any string possible):
       *     - thumb         : string - image filename
       *     - poster        : string - image filename
       *     - banner        : string - image filename
       *     - fanart        : string - image filename
       *     - clearart      : string - image filename
       *     - clearlogo     : string - image filename
       *     - landscape     : string - image filename
       *
       * example:
       *   - self.list.getSelectedItem().setArt({ 'poster': 'poster.png', 'banner' : 'banner.png' })
       */
      void setArt(const Properties& dictionary);

      /**
       * setUniqueIDs(values) -- Sets the listitem's uniqueID
       * \n
       * values              : dictionary - pairs of { label: value }.\n
       *
       * - Some example values (any string possible):
       *     - imdb          : string - uniqueid name
       *     - tvdb          : string - uniqueid name
       *     - tmdb          : string - uniqueid name
       *     - anidb         : string - uniqueid name
       *
       * example:
       *   - listitem.setUniqueIDs({ 'imdb': 'tt8938399', 'tmdb' : '9837493' })
       */
      void setUniqueIDs(const Properties& dictionary);

      /**
       * setRating(type, rating, votes = 0, defaultt = False) -- Sets a listitem's rating
       * \n
       * type                : string - the type of the rating. Any string.\n
       * rating              : float - the value of the rating.\n
       * votes               : int - the number of votes. Default 0.\n
       * defaultt            : bool - is the default rating? Default False.\n
       *
       * - Some example type (any string possible):
       *     - imdb          : string - rating type
       *     - tvdb          : string - rating type
       *     - tmdb          : string - rating type
       *     - anidb         : string - rating type
       *
       * example:
       *   - listitem.setRating("imdb", 4.6, 8940, True)
       */
      void setRating(std::string type, float rating, int votes = 0, bool defaultt = false);

      /**
       * addSeason(number, name = "") -- Adds a season with name to a listitem
       * \n
       * number              : int - the number of the season.\n
       * name                : string - the name of the season. Default "".\n
       *
       * example:
       *   - listitem.addSeason(1, "Murder House")
       *
       * @python_v18 New function added.
       */
      void addSeason(int number, std::string name = "");

      /**
       * getArt(key) -- Returns a listitem art path as a string, similar to an infolabel
       * \n
       * key                : string - art name.\n
       *
       * - Some default art values (any string possible):
       *     - thumb         : string - image path
       *     - poster        : string - image path
       *     - banner        : string - image path
       *     - fanart        : string - image path
       *     - clearart      : string - image path
       *     - clearlogo     : string - image path
       *     - landscape     : string - image path
       *     - icon          : string - image path
       *
       * example:
       *   - poster = listitem.getArt('poster')
       *
       * @python_v17 New function added.
       */
      String getArt(const char* key);

      /**
       * getUniqueID(key) -- Returns a listitem uniqueID as a string, similar to an infolabel
       * \n
       * key                : string - uniqueID name.\n
       *
       * - Some default uniqueID values (any string possible):
       *     - imdb          : string - uniqueid name
       *     - tvdb          : string - uniqueid name
       *     - tmdb          : string - uniqueid name
       *     - anidb         : string - uniqueid name
       *
       * example:
       *   - uniqueID = listitem.getUniqueID('imdb')
       */
      String getUniqueID(const char* key);

      /**
       * getRating(key) -- Returns a listitem rating as a float
       * \n
       * key                : string - rating type.\n
       *
       * - Some default key values (any string possible):
       *     - imdb          : string - type name
       *     - tvdb          : string - type name
       *     - tmdb          : string - type name
       *     - anidb         : string - type name
       *
       * example:
       *   - rating = listitem.getRating('imdb')
       */
      float getRating(const char* key);

      /**
       * getVotes(key) -- Returns a listitem votes as an integer
       * \n
       * key                : string - rating type.\n
       *
       * - Some default key values (any string possible):
       *     - imdb          : string - type name
       *     - tvdb          : string - type name
       *     - tmdb          : string - type name
       *     - anidb         : string - type name
       *
       * example:
       *   - votes = listitem.getVotes('imdb')
       */
      int getVotes(const char* key);

      /**
       * select(selected) -- Sets the listitem's selected status.\n
       * \n
       * selected        : bool - True=selected/False=not selected\n
       * \n
       * example:
       *   - self.list.getSelectedItem().select(True)
       */
      void select(bool selected);

      /**
       * isSelected() -- Returns the listitem's selected status.\n
       * \n
       * example:
       *   - is = self.list.getSelectedItem().isSelected()
       */
      bool isSelected();

      /**
       * setInfo(type, infoLabels) -- Sets the listitem's infoLabels.\n
       * \n
       * type              : string - type of media(video/music/pictures).\n
       * infoLabels        : dictionary - pairs of { label: value }.\n
       * \n
       * *Note, To set pictures exif info, prepend 'exif:' to the label. Exif values must be passed\n
       *        as strings, separate value pairs with a comma. (eg. {'exif:resolution': '720,480'}\n
       *        See CPictureInfoTag::TranslateString in PictureInfoTag.cpp for valid strings.\n
       * \n
       *        You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * \n
       * - General Values that apply to all types:
       *     - count         : integer (12) - can be used to store an id for later, or for sorting purposes
       *     - size          : long (1024) - size in bytes
       *     - date          : string (%d.%m.%Y / 01.01.2009) - file date
       * - Video Values:
       *     - genre         : string (Comedy) or list of strings (["Comedy", "Animation", "Drama"])
       *     - country       : string (Germany) or list of strings (["Germany", "Italy", "France"])
       *     - year          : integer (2009)
       *     - episode       : integer (4)
       *     - season        : integer (1)
       *     - sortepisode   : integer (4)
       *     - sortseason    : integer (1)
       *     - episodeguide  : string (Episode guide)
       *     - showlink      : string (Battlestar Galactica) or list of strings (["Battlestar Galactica", "Caprica"])
       *     - top250        : integer (192)
       *     - tracknumber   : integer (3)
       *     - rating        : float (6.4) - range is 0..10
       *     - watched       : depreciated - use playcount instead
       *     - playcount     : integer (2) - number of times this item has been played
       *     - overlay       : integer (2) - range is 0..8.  See GUIListItem.h for values
       *     - cast          : list (Michal C. Hall)
       *     - castandrole   : list (Michael C. Hall|Dexter)
       *     - director      : string (Dagur Kari) or list of strings (["Dagur Kari", "Quentin Tarantino", "Chrstopher Nolan"])
       *     - mpaa          : string (PG-13)
       *     - plot          : string (Long Description)
       *     - plotoutline   : string (Short Description)
       *     - title         : string (Big Fan)
       *     - originaltitle : string (Big Fan)
       *     - sorttitle     : string (Big Fan)
       *     - duration      : string (3:18)
       *     - studio        : string (Warner Bros.) or list of strings (["Warner Bros.", "Disney", "Paramount"])
       *     - tagline       : string (An awesome movie) - short description of movie
       *     - writer        : string (Robert D. Siegel) or list of strings (["Robert D. Siegel", "Jonathan Nolan", "J.K. Rowling"])
       *     - tvshowtitle   : string (Heroes)
       *     - premiered     : string (2005-03-04)
       *     - status        : string (Continuing) - status of a TVshow
       *     - setoverview   : string (All Batman movies) - overview of the collection
       *     - tag           : string (cult) - or list of strings (["cult", "documentary", "best movies"]) movie tag
       *     - code          : string (tt0110293) - IMDb code
       *     - aired         : string (2008-12-07)
       *     - credits       : string (Andy Kaufman) or list of strings (["Dagur Kari", "Quentin Tarantino", "Chrstopher Nolan"]) - writing credits
       *     - lastplayed    : string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
       *     - album         : string (The Joshua Tree)
       *     - artist        : list (['U2'])
       *     - votes         : string (12345 votes)
       *     - trailer       : string (/home/user/trailer.avi)
       *     - dateadded     : string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
       * - Music Values:
       *     - tracknumber   : integer (8)
       *     - duration      : integer (245) - duration in seconds
       *     - year          : integer (1998)
       *     - genre         : string (Rock)
       *     - album         : string (Pulse)
       *     - artist        : string (Muse)
       *     - title         : string (American Pie)
       *     - rating        : string (3) - single character between 0 and 5
       *     - lyrics        : string (On a dark desert highway...)
       *     - playcount     : integer (2) - number of times this item has been played
       *     - lastplayed    : string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
       * - Picture Values:
       *     - title         : string (In the last summer-1)
       *     - picturepath   : string (/home/username/pictures/img001.jpg)
       *     - exif*         : string (See CPictureInfoTag::TranslateString in PictureInfoTag.cpp for valid strings)
       * 
       * example:\n
       *   - self.list.getSelectedItem().setInfo('video', { 'Genre': 'Comedy' })n\n
       */
      void setInfo(const char* type, const InfoLabelDict& infoLabels) throw (WrongTypeException);

      /**
       * setCast(actors) -- Sets cast including thumbnails
       * \n
       * actors             : list of dictionaries (see below for relevant keys).\n
       *
       * - Keys:
       *     - name          : string (Michael C. Hall)
       *     - role          : string (Dexter)
       *     - thumbnail     : string (http://www.someurl.com/someimage.png)
       *     - order         : integer (1)
       *
       * example:
       *   - actors = [{"name": "Actor 1", "role": "role 1"}, {"name": "Actor 2", "role": "role 2"}]
       *   - listitem.setCast(actors)
       *
       * @python_v17 New function added.
       */
      void setCast(const std::vector<Properties>& actors);

      /**
       * setAvailableFanart(images) -- Sets available images (needed for scrapers)
       * \n
       * images              : list of dictionaries (see below for relevant keys).\n
       *
       * - Keys:
       *     - image         : string (http://www.someurl.com/someimage.png)
       *     - preview       : [opt] string (http://www.someurl.com/somepreviewimage.png)
       *     - colors        : [opt] string (either comma separated Kodi hex values ("FFFFFFFF,DDDDDDDD") or TVDB RGB Int Triplets ("|68,69,59|69,70,58|78,78,68|"))
       *
       * example:
       *   - fanart = [{"image": path_to_image_1, "preview": path_to_preview_1}, {"image": path_to_image_2, "preview": path_to_preview_2}]
       *   - listitem.setAvailableFanart(fanart)
       *
       * @python_v18 New function added.
       */
      void setAvailableFanart(const std::vector<Properties>& images);

      /**
       * addAvailableThumb(url, aspect = "", referrer = "", cache = "", post = False, isgz = False, season = 0) -- Adds a thumb to available thumbs (needed for scrapers)
       * \n
       * url                 : string - image path URL.\n
       * aspect              : [opt] string - image type.\n
       * referrer            : [opt] string - referrer URL.\n
       * cache               : [opt] string - filename in cache.\n
       * post                : [opt] bool - use POST to retrieve the image, default False.\n
       * isgz                : [opt] bool - use GZIP to retrieve the image, default False.\n
       * season              : [opt] integer - number of season in case of season thumb.\n
       *
       * example:
       *   - listitem.addAvailableThumb(path_to_image_1, "1.77")
       *
       * @python_v18 New function added.
       */
      void addAvailableThumb(std::string url, std::string aspect = "", std::string referrer = "", std::string cache = "", bool post = false, bool isgz = false, int season = -1);

      /**
       * addStreamInfo(type, values) -- Add a stream with details.\n
       * \n
       * type              : string - type of stream(video/audio/subtitle).\n
       * values            : dictionary - pairs of { label: value }.\n
       * 
       * - Video Values:
       *     - codec         : string (h264)
       *     - aspect        : float (1.78)
       *     - width         : integer (1280)
       *     - height        : integer (720)
       *     - duration      : integer (seconds)
       * - Audio Values:
       *     - codec         : string (dts)
       *     - language      : string (en)
       *     - channels      : integer (2)
       * - Subtitle Values:
       *     - language      : string (en)
       * 
       * example:
       *   - self.list.getSelectedItem().addStreamInfo('video', { 'Codec': 'h264', 'Width' : 1280 })
       */
      void addStreamInfo(const char* cType, const Properties& dictionary);

      /**
       * addContextMenuItems([(label, action,)*], replaceItems) -- Adds item(s) to the context menu for media lists.\n
       * \n
       * items               : list - [(label, action,)*] A list of tuples consisting of label and action pairs.
       *   - label           : string or unicode - item's label.
       *   - action          : string or unicode - any built-in function to perform.
       * replaceItems        : [opt] bool - Deprecated.
       * \n
       * List of functions - http://wiki.xbmc.org/?title=List_of_Built_In_Functions \n
       * \n
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * \n
       * example:
       *   - listitem.addContextMenuItems([('Theater Showtimes', 'XBMC.RunScript(special://home/scripts/showtimes/default.py,Iron Man)',)])n
       */
      void addContextMenuItems(const std::vector<Tuple<String,String> >& items, bool replaceItems = false) throw (ListItemException);

      /**
       * setProperty(key, value) -- Sets a listitem property, similar to an infolabel.\n
       * \n
       * key            : string - property name.\n
       * value          : string or unicode - value of property.\n
       * \n
       * *Note, Key is NOT case sensitive.\n
       *        You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * \n
       *  Some of these are treated internally by XBMC, such as the 'StartOffset' property, which is\n
       *  the offset in seconds at which to start playback of an item.  Others may be used in the skin\n
       *  to add extra information, such as 'WatchedCount' for tvshow items\n
       * 
       * example:
       *   - self.list.getSelectedItem().setProperty('AspectRatio', '1.85 : 1')
       *   - self.list.getSelectedItem().setProperty('StartOffset', '256.4')
       */
      void setProperty(const char * key, const String& value);

      /**
       * getProperty(key) -- Returns a listitem property as a string, similar to an infolabel.\n
       * \n
       * key            : string - property name.\n
       * \n
       * *Note, Key is NOT case sensitive.\n
       *        You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * 
       * example:
       *   - AspectRatio = self.list.getSelectedItem().getProperty('AspectRatio')
       */
      String getProperty(const char* key);

      /**
       * addContextMenuItems([(label, action,)*], replaceItems) -- Adds item(s) to the context menu for media lists.\n
       * \n
       * items               : list - [(label, action,)*] A list of tuples consisting of label and action pairs.
       *   - label           : string or unicode - item's label.
       *   - action          : string or unicode - any built-in function to perform.
       * replaceItems        : [opt] bool - True=only your items will show/False=your items will be added to context menu(Default).
       * \n
       * List of functions - http://wiki.xbmc.org/?title=List_of_Built_In_Functions \n
       * \n
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * \n
       * example:
       *   - listitem.addContextMenuItems([('Theater Showtimes', 'XBMC.RunScript(special://home/scripts/showtimes/default.py,Iron Man)',)])
       */
      //    void addContextMenuItems();

      /**
       * setPath(path) -- Sets the listitem's path.\n
       * \n
       * path           : string or unicode - path, activated when item is clicked.\n
       * \n
       * *Note, You can use the above as keywords for arguments.\n
       * 
       * example:
       *   - self.list.getSelectedItem().setPath(path='ActivateWindow(Weather)')
       */
      void setPath(const String& path);

      /**
       * setMimeType(mimetype) -- Sets the listitem's mimetype if known.\n
       * \n
       * mimetype           : string or unicode - mimetype.\n
       * \n
       * If known prehand, this can (but does not have to) avoid HEAD requests
       * being sent to HTTP servers to figure out file type.\n
       */
      void setMimeType(const String& mimetype);

      /**
       * setContentLookup(enable) -- Enable or disable content lookup for item.
       *
       * If disabled, HEAD requests to e.g determine mime type will not be sent.
       *
       * enable : bool
       */
      void setContentLookup(bool enable);

      /**
       * setSubtitles(subtitleFiles) -- Sets subtitles for this listitem
       * \n
       * subtitleFiles       : list with paths to subtitle files.\n
       *
       * example:
       *   - listitem.setSubtitles(['special://temp/example.srt', 'http://example.com/example.srt'])
       *
       * @python_v14 New function added.
       */
      void setSubtitles(const std::vector<String>& subtitleFiles);

      /**
       * getdescription() -- Returns the description of this PlayListItem.\n
       */
      String getdescription();
      
      /**
       * getduration() -- Returns the duration of this PlayListItem\n
       */
      String getduration();

      /**
       * getfilename() -- Returns the filename of this PlayListItem.\n
       */
      String getfilename();

      /**
       * getVideoInfoTag() -- returns the VideoInfoTag for this item.
       */
      xbmc::InfoTagVideo* getVideoInfoTag();

      /**
       * getMusicInfoTag() -- returns the MusicInfoTag for this item.
       */
      xbmc::InfoTagMusic* getMusicInfoTag();

private:
      std::vector<std::string> getStringArray(const InfoLabelValue& alt, const std::string& tag, std::string value = "");
    };

    typedef std::vector<ListItem*> ListItemList;
    
  }
}


