# -*- coding: UTF-8 -*-
#
# Copyright (C) 2020, Team Kodi
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# pylint: disable=missing-docstring
#
# This is based on the metadata.tvmaze scrapper by Roman Miroshnychenko aka Roman V.M.

u"""Functions to process data"""

from __future__ import absolute_import, unicode_literals

import re, json
from collections import OrderedDict, namedtuple
from .utils import safe_get, logger
from . import settings

try:
    from typing import Optional, Text, Dict, List, Any  # pylint: disable=unused-import
    from xbmcgui import ListItem  # pylint: disable=unused-import
    InfoType = Dict[Text, Any]  # pylint: disable=invalid-name
except ImportError:
    pass

TAG_RE = re.compile(ur'<[^>]+>')
SHOW_ID_REGEXPS = (
    ur'(tvmaze)\.com/shows/(\d+)/[\w\-]',
    ur'(thetvdb)\.com/.*?series/(\d+)',
    ur'(thetvdb)\.com[\w=&\?/]+id=(\d+)',
    ur'(imdb)\.com/[\w/\-]+/(tt\d+)',
    ur'(themoviedb)\.org/tv/(\d+).*/episode_group/(.*)',
    ur'(themoviedb)\.org/tv/(\d+)',
    ur'(themoviedb)\.org/./tv/(\d+)',
    ur'(tmdb)\.org/./tv/(\d+)'
)
SUPPORTED_ARTWORK_TYPES = set([u'poster', u'banner'])
IMAGE_SIZES = (u'large', u'original', u'medium')
CLEAN_PLOT_REPLACEMENTS = (
    (u'<b>', u'[B]'),
    (u'</b>', u'[/B]'),
    (u'<i>', u'[I]'),
    (u'</i>', u'[/I]'),
    (u'</p><p>', u'[CR]'),
)
VALIDEXTIDS = [u'tmdb_id', u'imdb_id', u'tvdb_id']

UrlParseResult = namedtuple(u'UrlParseResult', [u'provider', u'show_id', u'ep_grouping'])



def _clean_plot(plot):
    # type: (Text) -> Text
    u"""Replace HTML tags with Kodi skin tags"""
    for repl in CLEAN_PLOT_REPLACEMENTS:
        plot = plot.replace(repl[0], repl[1])
    plot = TAG_RE.sub(u'', plot)
    return plot


def _set_cast(cast_info, list_item):
    # type: (InfoType, ListItem) -> ListItem
    u"""Save cast info to list item"""
    cast = []
    for item in cast_info:
        data = {
            u'name': item[u'name'],
            u'role': item.get(u'character', item.get(u'character_name', u'')),
            u'order': item[u'order'],
        }
        thumb = None
        if safe_get(item, u'profile_path') is not None:
            thumb = settings.IMAGEROOTURL + item[u'profile_path']
        if thumb:
            data[u'thumbnail'] = thumb
        cast.append(data)
    list_item.setCast(cast)
    return list_item


def _get_credits(show_info):
    # type: (InfoType) -> List[Text]
    u"""Extract show creator(s) and writer(s) from show info"""
    credits = []
    for item in show_info.get(u'created_by', []):
        credits.append(item[u'name'])
    for item in show_info.get(u'credits', {}).get(u'crew', []):
        isWriter = item.get(u'job', u'').lower() == u'writer' or item.get(u'department', u'').lower() == u'writing'
        if isWriter and item.get(u'name') not in credits:
            credits.append(item[u'name'])
    return credits


def _get_directors(episode_info):
    # type: (InfoType) -> List[Text]
    u"""Extract episode writer(s) from episode info"""
    directors_ = []
    for item in episode_info.get(u'credits', {}).get(u'crew', []):
        if item.get(u'job') == u'Director':
            directors_.append(item[u'name'])
    return directors_


def _set_unique_ids(ext_ids, list_item):
    # type: (InfoType, ListItem) -> ListItem
    u"""Extract unique ID in various online databases"""
    unique_ids = {}
    for key, value in ext_ids.items():
        if key in VALIDEXTIDS and value:
            key = key[:-3]
            unique_ids[key] = unicode(value)
    list_item.setUniqueIDs(unique_ids, u'tmdb')
    return list_item


def _set_rating(the_info, list_item, episode=False):
    # type: (InfoType, ListItem) -> ListItem
    u"""Set show/episode rating"""
    first = True
    for rating_type in settings.RATING_TYPES:
        logger.debug(u'adding rating type of %s' % rating_type)
        rating = float(the_info.get(u'ratings', {}).get(rating_type, {}).get(u'rating', u'0'))
        votes = int(the_info.get(u'ratings', {}).get(rating_type, {}).get(u'votes', u'0'))
        logger.debug(u"adding rating of %s and votes of %s" % (unicode(rating), unicode(votes)))
        if rating > 0:
            list_item.setRating(rating_type, rating, votes=votes, defaultt=first)
            first = False
    return list_item


def _add_season_info(show_info, list_item):
    # type: (InfoType, ListItem) -> ListItem
    u"""Add info for show seasons"""
    for season in show_info[u'seasons']:
        logger.debug(u'adding information for season %s to list item' % season[u'season_number'])
        list_item.addSeason(season[u'season_number'], safe_get(season, u'name', u''))
        for image_type, image_list in season.get(u'images', {}).items():
            if image_type == u'posters':
                destination = u'poster'
            else:
                destination = image_type
            for image in image_list:
                if image.get(u'type') == u'fanarttv':
                    theurl = image[u'file_path']
                    previewurl = theurl.replace(u'.fanart.tv/fanart/', u'.fanart.tv/preview/')
                else:
                    theurl = settings.IMAGEROOTURL + image[u'file_path']
                    previewurl = settings.PREVIEWROOTURL + image[u'file_path']
                list_item.addAvailableArtwork(theurl, art_type=destination, preview=previewurl, season=season[u'season_number'])
    return list_item


def get_image_urls( image ):
    if image.get(u'type') == u'fanarttv':
        theurl = image[u'file_path']
        previewurl = theurl.replace(u'.fanart.tv/fanart/', u'.fanart.tv/preview/')
    else:
        theurl = settings.IMAGEROOTURL + image[u'file_path']
        previewurl = settings.PREVIEWROOTURL + image[u'file_path']
    return theurl, previewurl


def set_show_artwork(show_info, list_item):
    # type: (InfoType, ListItem) -> ListItem
    u"""Set available images for a show"""
    for image_type, image_list in show_info.get(u'images', {}).items():
        if image_type == u'backdrops':
            fanart_list = []
            for image in image_list:
                if image.get(u'type') == u'fanarttv':
                    theurl = image[u'file_path']
                else:
                    theurl = settings.IMAGEROOTURL + image[u'file_path']
                if image.get(u'iso_639_1') != None and settings.CATLANDSCAPE:
                    theurl, previewurl = get_image_urls( image )
                    list_item.addAvailableArtwork(theurl, art_type=u"landscape", preview=previewurl)
                else:
                    fanart_list.append({u'image': theurl})
            if fanart_list:
                list_item.setAvailableFanart(fanart_list)
        else:
            if image_type == u'posters':
                destination = u'poster'
            else:
                destination = image_type
            for image in image_list:
                theurl, previewurl = get_image_urls( image )
                list_item.addAvailableArtwork(theurl, art_type=destination, preview=previewurl)
    return list_item


def add_main_show_info(list_item, show_info, full_info=True):
    # type: (ListItem, InfoType, bool) -> ListItem
    u"""Add main show info to a list item"""
    plot = _clean_plot(safe_get(show_info, u'overview', u''))
    original_name = show_info.get(u'original_name')
    if settings.KEEPTITLE and original_name:
        showname = original_name
    else:
        showname = show_info[u'name']
    video = {
        u'plot': plot,
        u'plotoutline': plot,
        u'title': showname,
        u'originaltitle': original_name,
        u'tvshowtitle': showname,
        u'mediatype': u'tvshow',
        # This property is passed as "url" parameter to getepisodelist call
        u'episodeguide': unicode(show_info[u'id']),
    }
    if show_info.get(u'first_air_date'):
        video[u'year'] = int(show_info[u'first_air_date'][:4])
        video[u'premiered'] = show_info[u'first_air_date']
    if full_info:
        video[u'status'] = safe_get(show_info, u'status', u'')
        genre_list = safe_get(show_info, u'genres', {})
        genres = []
        for genre in genre_list:
            genres.append(genre[u'name'])
        video[u'genre'] = genres
        networks = show_info.get(u'networks', [])
        if networks:
            network = networks[0]
            country = network.get(u'origin_country', u'')
        else:
            network = None
            country = None
        if network and country:
            video[u'studio'] = u'{0} ({1})'.format(network[u'name'], country)
            video[u'country'] = country
        content_ratings = show_info.get(u'content_ratings', {}).get(u'results', {})
        if content_ratings:
            mpaa = u''
            mpaa_backup = u''
            for content_rating in content_ratings:
                iso = content_rating.get(u'iso_3166_1', u'').lower()
                if iso == u'us':
                    mpaa_backup = content_rating.get(u'rating')
                if iso == settings.CERT_COUNTRY.lower():
                    mpaa = content_rating.get(u'rating', u'')
            if not mpaa:
                mpaa = mpaa_backup
            if mpaa:
                video[u'Mpaa'] = settings.CERT_PREFIX + mpaa
        video[u'credits'] = video[u'writer'] = _get_credits(show_info)
        list_item = set_show_artwork(show_info, list_item)
        list_item = _add_season_info(show_info, list_item)
        list_item = _set_cast(show_info[u'credits'][u'cast'], list_item)
        list_item = _set_rating(show_info, list_item)
        ext_ids = {u'tmdb_id': show_info[u'id']}
        ext_ids.update(show_info.get(u'external_ids', {}))
        list_item =  _set_unique_ids(ext_ids, list_item)
    else:
        image = safe_get(show_info, u'poster_path', u'')
        if image:
            theurl = settings.IMAGEROOTURL + image
            previewurl = settings.PREVIEWROOTURL + image
            list_item.addAvailableArtwork(theurl, art_type=u'poster')
    logger.debug(u'adding tv show information for %s to list item' % video[u'tvshowtitle'])
    list_item.setInfo(u'video', video)
    # This is needed for getting artwork
    list_item = _set_unique_ids(show_info, list_item)
    return list_item


def add_episode_info(list_item, episode_info, full_info=True):
    # type: (ListItem, InfoType, bool) -> ListItem
    u"""Add episode info to a list item"""
    video = {
        u'title': episode_info.get(u'name', u'Episode ' + unicode(episode_info[u'episode_number'])),
        u'season': episode_info[u'season_number'],
        u'episode': episode_info[u'episode_number'],
        u'mediatype': u'episode',
    }
    if safe_get(episode_info, u'air_date') is not None:
        video[u'aired'] = episode_info[u'air_date']
    if full_info:
        summary = safe_get(episode_info, u'overview')
        if summary is not None:
            video[u'plot'] = video[u'plotoutline'] = _clean_plot(summary)
        if safe_get(episode_info, u'air_date') is not None:
            video[u'premiered'] = episode_info[u'air_date']
        list_item = _set_cast(episode_info[u'credits'][u'guest_stars'], list_item)
        ext_ids = {u'tmdb_id': episode_info[u'id']}
        ext_ids.update(episode_info.get(u'external_ids', {}))
        list_item = _set_unique_ids(ext_ids, list_item)
        list_item = _set_rating(episode_info, list_item, episode=True)
        for image in episode_info.get(u'images', {}).get(u'stills', []):
            img_path = image.get(u'file_path')
            if img_path:
                theurl = settings.IMAGEROOTURL + img_path
                previewurl = settings.PREVIEWROOTURL + img_path
                list_item.addAvailableArtwork(theurl, art_type=u'thumb', preview=previewurl)
        video[u'credits'] = video[u'writer'] = _get_credits(episode_info)
        video[u'director'] = _get_directors(episode_info)
    logger.debug(u'adding episode information for S%sE%s - %s to list item' % (video[u'season'], video[u'episode'], video[u'title']))
    list_item.setInfo(u'video', video)
    return list_item


def parse_nfo_url(nfo):
    # type: (Text) -> Optional[UrlParseResult]
    u"""Extract show ID and named seasons from NFO file contents"""
    # work around for xbmcgui.ListItem.addSeason overwriting named seasons from NFO files
    ns_regex = ur'<namedseason number="(.*)">(.*)</namedseason>'
    ns_match = re.findall(ns_regex, nfo, re.I)
    sid_match = None
    for regexp in SHOW_ID_REGEXPS:
        logger.debug(u'trying regex to match service from parsing nfo:')
        logger.debug(regexp)
        show_id_match = re.search(regexp, nfo, re.I)
        if show_id_match:
            logger.debug(u'match group 1: ' + show_id_match.group(1))
            logger.debug(u'match group 2: ' + show_id_match.group(2))
            try:
                ep_grouping = show_id_match.group(3)
            except IndexError:
                ep_grouping = None
            if ep_grouping is not None:
                logger.debug(u'match group 3: ' + ep_grouping)
            else:
                logger.debug(u'match group 3: None')
            sid_match = UrlParseResult(show_id_match.group(1), show_id_match.group(2), ep_grouping)
            break
    return sid_match, ns_match


def parse_media_id(title):
    title = title.lower()
    if title.startswith(u'tt') and title[2:].isdigit():
        return {u'type': u'imdb_id', u'title': title} # IMDB ID works alone because it is clear
    elif title.startswith(u'imdb/tt') and title[7:].isdigit(): # IMDB ID with prefix to match
        return {u'type': u'imdb_id', u'title': title[5:]} # IMDB ID works alone because it is clear
    elif title.startswith(u'tmdb/') and title[5:].isdigit(): # TVDB ID
        return {u'type': u'tmdb_id', u'title': title[5:]}
    elif title.startswith(u'tvdb/') and title[5:].isdigit(): # TVDB ID
        return {u'type': u'tvdb_id', u'title': title[5:]}
    return None
