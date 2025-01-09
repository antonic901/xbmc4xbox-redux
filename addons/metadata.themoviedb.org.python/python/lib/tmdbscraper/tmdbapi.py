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

u"""Functions to interact with TMDb API."""

from __future__ import absolute_import
from . import api_utils
import xbmc
try:
    from typing import Optional, Text, Dict, List, Any  # pylint: disable=unused-import
    InfoType = Dict[Text, Any]  # pylint: disable=invalid-name
except ImportError:
    pass


HEADERS = (
    (u'User-Agent', u'Kodi Movie scraper by Team Kodi'),
    (u'Accept', u'application/json'),
)
api_utils.set_headers(dict(HEADERS))

TMDB_PARAMS = {u'api_key': u'f090bb54758cabf231fb605d3e3e0468'}
BASE_URL = u'https://api.themoviedb.org/3/{}'
SEARCH_URL = BASE_URL.format(u'search/movie')
FIND_URL = BASE_URL.format(u'find/{}')
MOVIE_URL = BASE_URL.format(u'movie/{}')
COLLECTION_URL = BASE_URL.format(u'collection/{}')
CONFIG_URL = BASE_URL.format(u'configuration')


def search_movie(query, year=None, language=None):
    # type: (Text) -> List[InfoType]
    u"""
    Search for a movie

    :param title: movie title to search
    :param year: the year to search (optional)
    :param language: the language filter for TMDb (optional)
    :return: a list with found movies
    """
    xbmc.log(u'using title of %s to find movie' % query, xbmc.LOGDEBUG)
    theurl = SEARCH_URL
    params = _set_params(None, language)
    params[u'query'] = query
    if year is not None:
        params[u'year'] = unicode(year)
    return api_utils.load_info(theurl, params=params)


def find_movie_by_external_id(external_id, language=None):
    # type: (Text) -> List[InfoType]
    u"""
    Find movie based on external ID

    :param mid: external ID
    :param language: the language filter for TMDb (optional)
    :return: the movie or error
    """
    xbmc.log(u'using external id of %s to find movie' % external_id, xbmc.LOGDEBUG)
    theurl = FIND_URL.format(external_id)
    params = _set_params(None, language)
    params[u'external_source'] = u'imdb_id'
    return api_utils.load_info(theurl, params=params)



def get_movie(mid, language=None, append_to_response=None):
    # type: (Text) -> List[InfoType]
    u"""
    Get movie details

    :param mid: TMDb movie ID
    :param language: the language filter for TMDb (optional)
    :append_to_response: the additional data to get from TMDb (optional)
    :return: the movie or error
    """
    xbmc.log(u'using movie id of %s to get movie details' % mid, xbmc.LOGDEBUG)
    theurl = MOVIE_URL.format(mid)
    return api_utils.load_info(theurl, params=_set_params(append_to_response, language))


def get_collection(collection_id, language=None, append_to_response=None):
    # type: (Text) -> List[InfoType]
    u"""
    Get movie collection information

    :param collection_id: TMDb collection ID
    :param language: the language filter for TMDb (optional)
    :append_to_response: the additional data to get from TMDb (optional)
    :return: the movie or error
    """
    xbmc.log(u'using collection id of %s to get collection details' % collection_id, xbmc.LOGDEBUG)
    theurl = COLLECTION_URL.format(collection_id)
    return api_utils.load_info(theurl, params=_set_params(append_to_response, language))


def get_configuration():
    # type: (Text) -> List[InfoType]
    u"""
    Get configuration information

    :return: configuration details or error
    """
    xbmc.log(u'getting configuration details', xbmc.LOGDEBUG)
    return api_utils.load_info(CONFIG_URL, params=TMDB_PARAMS.copy())


def _set_params(append_to_response, language):
    params = TMDB_PARAMS.copy()
    img_lang = u'en,null'
    if language is not None:
        params[u'language'] = language
        img_lang = u'%s,en,null' % language[0:2]
    if append_to_response is not None:
        params[u'append_to_response'] = append_to_response
        if u'images' in append_to_response:
            params[u'include_image_language'] = img_lang
    return params
