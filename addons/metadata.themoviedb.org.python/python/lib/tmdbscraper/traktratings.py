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

u"""Functions to interact with Trakt API."""

from __future__ import absolute_import, unicode_literals

from . import api_utils
from . import get_imdb_id
try:
    from typing import Optional, Text, Dict, List, Any  # pylint: disable=unused-import
    InfoType = Dict[Text, Any]  # pylint: disable=invalid-name
except ImportError:
    pass


HEADERS = (
    (u'User-Agent', u'Kodi Movie scraper by Team Kodi'),
    (u'Accept', u'application/json'),
    (u'trakt-api-key', u'5f2dc73b6b11c2ac212f5d8b4ec8f3dc4b727bb3f026cd254d89eda997fe64ae'),
    (u'trakt-api-version', u'2'),
    (u'Content-Type', u'application/json'),
)
api_utils.set_headers(dict(HEADERS))

MOVIE_URL = u'https://api.trakt.tv/movies/{}'


def get_trakt_ratinginfo(uniqueids):
    imdb_id = get_imdb_id(uniqueids)
    result = {}
    url = MOVIE_URL.format(imdb_id)
    params = {u'extended': u'full'}
    movie_info = api_utils.load_info(url, params=params, default={})
    if(movie_info):
        if u'votes' in movie_info and u'rating' in movie_info:
            result[u'ratings'] = {u'trakt': {u'votes': int(movie_info[u'votes']), u'rating': float(movie_info[u'rating'])}}
        elif u'rating' in movie_info:
            result[u'ratings'] = {u'trakt': {u'rating': float(movie_info[u'rating'])}}
    return result
