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

u"""Functions to interact with TMDb API"""

from __future__ import division
from __future__ import absolute_import, unicode_literals

import unicodedata
from math import floor
from pprint import pformat
from . import cache, data_utils, api_utils, settings, imdbratings, traktratings
from .utils import logger
try:
    from typing import Text, Optional, Union, List, Dict, Any  # pylint: disable=unused-import
    InfoType = Dict[Text, Any]  # pylint: disable=invalid-name
except ImportError:
    pass

HEADERS = (
    (u'User-Agent', u'Kodi TV Show scraper by Team Kodi; contact pkscout@kodi.tv'),
    (u'Accept', u'application/json'),
)
api_utils.set_headers(dict(HEADERS))

TMDB_PARAMS = {u'api_key': settings.TMDB_CLOWNCAR, u'language': settings.LANG}
BASE_URL = u'https://api.themoviedb.org/3/{}'
EPISODE_GROUP_URL = BASE_URL.format(u'tv/episode_group/{}')
SEARCH_URL = BASE_URL.format(u'search/tv')
FIND_URL = BASE_URL.format(u'find/{}')
SHOW_URL = BASE_URL.format(u'tv/{}')
SEASON_URL = BASE_URL.format(u'tv/{}/season/{}')
EPISODE_URL = BASE_URL.format(u'tv/{}/season/{}/episode/{}')
FANARTTV_URL = u'https://webservice.fanart.tv/v3/tv/{}'
FANARTTV_PARAMS = {u'api_key': settings.FANARTTV_CLOWNCAR}
if settings.FANARTTV_CLIENTKEY:
    FANARTTV_PARAMS[u'client_key'] = settings.FANARTTV_CLIENTKEY


def search_show(title, year=None):
    # type: (Text) -> List[InfoType]
    u"""
    Search for a single TV show

    :param title: TV show title to search
    : param year: the year to search (optional)
    :return: a list with found TV shows
    """
    params = TMDB_PARAMS.copy()
    results = []
    ext_media_id = data_utils.parse_media_id(title)
    if ext_media_id:
        logger.debug(u'using %s of %s to find show' % (ext_media_id[u'type'], ext_media_id[u'title']))
        if ext_media_id[u'type'] == u'tmdb_id':
            search_url = SHOW_URL.format(ext_media_id[u'title'])
        else:
            search_url = FIND_URL.format(ext_media_id[u'title'])
            params[u'external_source'] = ext_media_id[u'type']
    else:
        logger.debug(u'using title of %s to find show' % title)
        search_url = SEARCH_URL
        params[u'query'] = unicodedata.normalize(u'NFKC', title)
        if year:
            params[u'first_air_date_year'] = unicode(year)
    resp = api_utils.load_info(search_url, params=params, verboselog=settings.VERBOSELOG)
    if resp is not None:
        if ext_media_id:
            if ext_media_id[u'type'] == u'tmdb_id':
                if resp.get(u'success') == u'false':
                    results = []
                else:
                    results = [resp]
            else:
                results = resp.get(u'tv_results', [])
        else:
            results = resp.get(u'results', [])
    return results


def load_episode_list(show_info, season_map, ep_grouping):
    # type: (Text) -> List[InfoType]
    u"""Load episode list from themoviedb.org API"""
    episode_list = []
    if ep_grouping is not None:
        logger.debug(u'Getting episodes with episode grouping of ' + ep_grouping)
        episode_group_url = EPISODE_GROUP_URL.format(ep_grouping)
        custom_order = api_utils.load_info(episode_group_url, params=TMDB_PARAMS, verboselog=settings.VERBOSELOG)
        if custom_order is not None:
            show_info[u'seasons'] = []
            for custom_season in custom_order.get(u'groups', []):
                season_episodes = []
                current_season = season_map.get(unicode(custom_season[u'episodes'][0][u'season_number']), {}).copy()
                current_season[u'name'] = custom_season[u'name']
                current_season[u'season_number'] = custom_season[u'order']
                for episode in custom_season[u'episodes']:
                    episode[u'org_seasonnum'] = episode[u'season_number']
                    episode[u'org_epnum'] = episode[u'episode_number']
                    episode[u'season_number'] = custom_season[u'order']
                    episode[u'episode_number'] = episode[u'order'] + 1
                    season_episodes.append(episode)
                    episode_list.append(episode)
                current_season[u'episodes'] = season_episodes
                show_info[u'seasons'].append(current_season)
    else:
        logger.debug(u'Getting episodes from standard season list')
        show_info[u'seasons'] = []
        for key, value in season_map.items():
            show_info[u'seasons'].append(value)
        for season in show_info.get(u'seasons', []):
            for episode in season.get(u'episodes', []):
                episode[u'org_seasonnum'] = episode[u'season_number']
                episode[u'org_epnum'] = episode[u'episode_number']
                episode_list.append(episode)
    show_info[u'episodes'] = episode_list
    return show_info


def load_show_info(show_id, ep_grouping=None, named_seasons=None):
    # type: (Text) -> Optional[InfoType]
    u"""
    Get full info for a single show

    :param show_id: themoviedb.org show ID
    :return: show info or None
    """
    if named_seasons == None:
        named_seasons = []
    show_info = cache.load_show_info_from_cache(show_id)
    if show_info is None:
        logger.debug(u'no cache file found, loading from scratch')
        show_url = SHOW_URL.format(show_id)
        params = TMDB_PARAMS.copy()
        params[u'append_to_response'] = u'credits,content_ratings,external_ids,images'
        params[u'include_image_language'] = u'%s,en,null' % settings.LANG[0:2]
        show_info = api_utils.load_info(show_url, params=params, verboselog=settings.VERBOSELOG)
        if show_info is None:
            return None
        if show_info[u'overview'] == u'' and settings.LANG != u'en-US':
            params[u'language'] = u'en-US'
            del params[u'append_to_response']
            show_info_backup = api_utils.load_info(show_url, params=params, verboselog=settings.VERBOSELOG)
            if show_info_backup is not None:
                show_info[u'overview'] = show_info_backup.get(u'overview', u'')
            params[u'language'] = settings.LANG
        season_map = {}
        params[u'append_to_response'] = u'credits,images'
        for season in show_info.get(u'seasons', []):
            season_url = SEASON_URL.format(show_id, season[u'season_number'])
            season_info = api_utils.load_info(season_url, params=params, default={}, verboselog=settings.VERBOSELOG)
            if (season_info[u'overview'] == u'' or season_info[u'name'].lower().startswith(u'season')) and settings.LANG != u'en-US':
                params[u'language'] = u'en-US'
                season_info_backup = api_utils.load_info(season_url, params=params, default={}, verboselog=settings.VERBOSELOG)
                params[u'language'] = settings.LANG
                if season_info[u'overview'] == u'':
                    season_info[u'overview'] = season_info_backup[u'overview']
                if season_info[u'name'].lower().startswith(u'season'):
                    season_info[u'name'] = season_info_backup[u'name']
            # this is part of a work around for xbmcgui.ListItem.addSeasons() not respecting NFO file information
            for named_season in named_seasons:
                if unicode(named_season[0]) == unicode(season[u'season_number']):
                    logger.debug(u'adding season name of %s from named seasons in NFO for season %s' % (named_season[1], season[u'season_number']))
                    season_info[u'name'] = named_season[1]
                    break
            # end work around
            season_info[u'images'] = _sort_image_types(season_info.get(u'images', {}))
            season_map[unicode(season[u'season_number'])] = season_info
        show_info = load_episode_list(show_info, season_map, ep_grouping)
        show_info[u'ratings'] = load_ratings(show_info)
        show_info = load_fanarttv_art(show_info)
        show_info[u'images'] = _sort_image_types(show_info.get(u'images', {}))
        show_info = trim_artwork(show_info)
        cast_check = []
        cast = []
        for season in reversed(show_info.get(u'seasons', [])):
            for cast_member in season.get(u'credits', {}).get(u'cast', []):
                if cast_member[u'name'] not in cast_check:
                    cast.append(cast_member)
                    cast_check.append(cast_member[u'name'])
        show_info[u'credits'][u'cast'] = cast
        logger.debug(u'saving show info to the cache')
        if settings.VERBOSELOG:
            logger.debug(format(pformat(show_info)))
        cache.cache_show_info(show_info)
    else:
        logger.debug(u'using cached show info')
    return show_info


def load_episode_info(show_id, episode_id):
    # type: (Text, Text) -> Optional[InfoType]
    u"""
    Load episode info

    :param show_id:
    :param episode_id:
    :return: episode info or None
    """
    show_info = load_show_info(show_id)
    if show_info is not None:
        try:
            episode_info = show_info[u'episodes'][int(episode_id)]
        except KeyError:
            return None
        # this ensures we are using the season/ep from the episode grouping if provided
        ep_url = EPISODE_URL.format(show_info[u'id'], episode_info[u'org_seasonnum'], episode_info[u'org_epnum'])
        params = TMDB_PARAMS.copy()
        params[u'append_to_response'] = u'credits,external_ids,images'
        params[u'include_image_language'] = u'%s,en,null' % settings.LANG[0:2]
        ep_return = api_utils.load_info(ep_url, params=params, verboselog=settings.VERBOSELOG)
        if ep_return is None:
            return None
        bad_return_name = False
        bad_return_overview = False
        check_name = ep_return.get(u'name')
        if check_name == None:
            bad_return_name = True
            ep_return[u'name'] = u'Episode ' + unicode(episode_info[u'episode_number'])
        elif check_name.lower().startswith(u'episode') or check_name == u'':
            bad_return_name = True
        if ep_return.get(u'overview', u'') == u'':
            bad_return_overview = True
        if (bad_return_overview or bad_return_name) and settings.LANG != u'en-US':
            params[u'language'] = u'en-US'
            del params[u'append_to_response']
            ep_return_backup = api_utils.load_info(ep_url, params=params, verboselog=settings.VERBOSELOG)
            if ep_return_backup is not None:
                if bad_return_overview:
                    ep_return[u'overview'] = ep_return_backup.get(u'overview', u'')
                if bad_return_name:
                    ep_return[u'name'] = ep_return_backup.get(u'name', u'Episode ' + unicode(episode_info[u'episode_number']))
        ep_return[u'images'] = _sort_image_types(ep_return.get(u'images', {}))
        ep_return[u'season_number'] = episode_info[u'season_number']
        ep_return[u'episode_number'] = episode_info[u'episode_number']
        ep_return[u'org_seasonnum'] = episode_info[u'org_seasonnum']
        ep_return[u'org_epnum'] = episode_info[u'org_epnum']
        ep_return[u'ratings'] = load_ratings(ep_return, show_imdb_id=show_info.get(u'external_ids', {}).get(u'imdb_id'))
        show_info[u'episodes'][int(episode_id)] = ep_return
        cache.cache_show_info(show_info)
        return ep_return
    return None


def load_ratings(the_info, show_imdb_id=u''):
    ratings = {}
    imdb_id = the_info.get(u'external_ids', {}).get(u'imdb_id')
    for rating_type in settings.RATING_TYPES:
        logger.debug(u'setting rating using %s' % rating_type)
        if rating_type == u'tmdb':
            ratings[u'tmdb'] = {u'votes': the_info[u'vote_count'], u'rating': the_info[u'vote_average']}
        elif rating_type == u'imdb' and imdb_id:
            imdb_rating = imdbratings.get_details(imdb_id).get(u'ratings')
            if imdb_rating:
                ratings.update(imdb_rating)
        elif rating_type == u'trakt':
            if show_imdb_id:
                season = the_info[u'org_seasonnum']
                episode = the_info[u'org_epnum']
                resp = traktratings.get_details(show_imdb_id, season=season, episode=episode)
            else:
                resp = traktratings.get_details(imdb_id)
            trakt_rating = resp.get(u'ratings')
            if trakt_rating:
                ratings.update(trakt_rating)
    logger.debug(u'returning ratings of\n{}'.format(pformat(ratings)))
    return ratings

def load_fanarttv_art(show_info):
    # type: (Text) -> Optional[InfoType]
    u"""
    Add fanart.tv images for a show

    :param show_info: the current show info
    :return: show info
    """
    tvdb_id = show_info.get(u'external_ids', {}).get(u'tvdb_id')
    if tvdb_id and settings.FANARTTV_ENABLE:
        fanarttv_url = FANARTTV_URL.format(tvdb_id)
        artwork = api_utils.load_info(fanarttv_url, params=FANARTTV_PARAMS, verboselog=settings.VERBOSELOG)
        if artwork is None:
            return show_info
        for fanarttv_type, tmdb_type in settings.FANARTTV_MAPPING.items():
            if not show_info[u'images'].get(tmdb_type) and not tmdb_type.startswith(u'season'):
                show_info[u'images'][tmdb_type] = []
            for item in artwork.get(fanarttv_type, []):
                lang = item.get(u'lang')
                if lang == u'' or lang == u'00':
                    lang = None
                filepath = u''
                if lang is None or lang == settings.LANG[0:2] or lang == u'en':
                    filepath = item.get(u'url')
                if filepath:
                    if tmdb_type.startswith(u'season'):
                        image_type = tmdb_type[6:]
                        for s in xrange(len(show_info.get(u'seasons', []))):
                            season_num = show_info[u'seasons'][s][u'season_number']
                            artseason = item.get(u'season', u'')
                            if not show_info[u'seasons'][s].get(u'images'):
                                show_info[u'seasons'][s][u'images'] = {}
                            if not show_info[u'seasons'][s][u'images'].get(image_type):
                                show_info[u'seasons'][s][u'images'][image_type] = []
                            if artseason == u'' or artseason == unicode(season_num):
                                show_info[u'seasons'][s][u'images'][image_type].append({u'file_path':filepath, u'type':u'fanarttv', u'iso_639_1': lang})
                    else:
                        show_info[u'images'][tmdb_type].append({u'file_path':filepath, u'type':u'fanarttv', u'iso_639_1': lang})
    return show_info


def trim_artwork(show_info):
    # type: (Text) -> Optional[InfoType]
    u"""
    Trim artwork to keep the text blob below 65K characters

    :param show_info: the current show info
    :return: show info
    """
    image_counts = {}
    image_total = 0
    backdrops_total = 0
    for image_type, image_list in show_info.get(u'images', {}).items():
        total = len(image_list)
        if image_type == u'backdrops':
            backdrops_total = backdrops_total + total
        else:
            image_counts[image_type] = {u'total':total}
            image_total = image_total + total
    for season in show_info.get(u'seasons', []):
        for image_type, image_list in season.get(u'images', {}).items():
            total = len(image_list)
            thetype = u'%s_%s' % (unicode(season[u'season_number']), image_type)
            image_counts[thetype] = {u'total':total}
            image_total = image_total + total
    if image_total <= settings.MAXIMAGES and backdrops_total <= settings.MAXIMAGES:
        return show_info
    if backdrops_total > settings.MAXIMAGES:
        logger.error(u'there are %s fanart images' % unicode(backdrops_total))
        logger.error(u'that is more than the max of %s, image results will be trimmed to the max' % unicode(settings.MAXIMAGES))
        reduce = -1 * (backdrops_total - settings.MAXIMAGES )
        del show_info[u'images'][u'backdrops'][reduce:]
    if image_total > settings.MAXIMAGES:
        reduction = (image_total - settings.MAXIMAGES)/image_total
        logger.error(u'there are %s non-fanart images' % unicode(image_total))
        logger.error(u'that is more than the max of %s, image results will be trimmed by %s' % (unicode(settings.MAXIMAGES), unicode(reduction)))
        for key, value in image_counts.items():
            total = value[u'total']
            reduce = int(floor(total * reduction))
            target = total - reduce
            if target < 5:
                reduce = 0
            else:
                reduce = -1 * reduce
            image_counts[key][u'reduce'] = reduce
            logger.debug(u'%s: %s' % (key, pformat(image_counts[key])))
        for image_type, image_list in show_info.get(u'images', {}).items():
            if image_type == u'backdrops':
                continue # already handled backdrops above
            reduce = image_counts[image_type][u'reduce']
            if reduce != 0:
                del show_info[u'images'][image_type][reduce:]
        for s in xrange(len(show_info.get(u'seasons', []))):
            for image_type, image_list in show_info[u'seasons'][s].get(u'images', {}).items():
                thetype = u'%s_%s' % (unicode(show_info[u'seasons'][s][u'season_number']), image_type)
                reduce = image_counts[thetype][u'reduce']
                if reduce != 0:
                    del show_info[u'seasons'][s][u'images'][image_type][reduce:]
    return show_info


def _sort_image_types(imagelist):
    for image_type, images in imagelist.items():
        imagelist[image_type] = _image_sort(images, image_type)
    return imagelist


def _image_sort(images, image_type):
    lang_pref = []
    lang_null = []
    lang_en = []
    firstimage = True
    for image in images:
        image_lang = image.get(u'iso_639_1')
        if image_lang == settings.LANG[0:2]:
            lang_pref.append(image)
        elif image_lang == u'en':
            lang_en.append(image)
        else:
            if firstimage:
                lang_pref.append(image)
            else:
                lang_null.append(image)
        firstimage = False
    if image_type == u'posters':
        return lang_pref + lang_en + lang_null
    else:
        return lang_pref + lang_null + lang_en
