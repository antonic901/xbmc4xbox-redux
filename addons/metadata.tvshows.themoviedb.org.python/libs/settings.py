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

from __future__ import absolute_import
import json, sys
import urllib2, urllib, urlparse
from .utils import logger
from . import api_utils
from xbmcaddon import Addon
from datetime import datetime, timedelta


def _get_date_numeric(datetime_):
    return (datetime_ - datetime(1970, 1, 1)).total_seconds()


def _get_configuration():
    logger.debug(u'getting configuration details')
    return api_utils.load_info(u'https://api.themoviedb.org/3/configuration', params={u'api_key': TMDB_CLOWNCAR}, verboselog=VERBOSELOG)


def _load_base_urls():
    image_root_url = ADDON.getSetting(u'originalUrl')
    preview_root_url = ADDON.getSetting(u'previewUrl')
    last_updated = ADDON.getSetting(u'lastUpdated')
    if not image_root_url or not preview_root_url or not last_updated or \
            float(last_updated) < _get_date_numeric(datetime.now() - timedelta(days=30)):
        conf = _get_configuration()
        if conf:
            image_root_url = conf[u'images'][u'secure_base_url'] + u'original'
            preview_root_url = conf[u'images'][u'secure_base_url'] + u'w780'
            ADDON.setSetting(u'originalUrl', image_root_url)
            ADDON.setSetting(u'previewUrl', preview_root_url)
            ADDON.setSetting(u'lastUpdated', unicode(_get_date_numeric(datetime.now())))
    return image_root_url, preview_root_url


ADDON = Addon()
TMDB_CLOWNCAR = u'af3a53eb387d57fc935e9128468b1899'
FANARTTV_CLOWNCAR = u'b018086af0e1478479adfc55634db97d'
TRAKT_CLOWNCAR = u'90901c6be3b2de5a4fa0edf9ab5c75e9a5a0fef2b4ee7373d8b63dcf61f95697'
MAXIMAGES = 350
FANARTTV_MAPPING = { u'showbackground': u'backdrops',
                     u'tvposter': u'posters',
                     u'tvbanner': u'banner',
                     u'hdtvlogo': u'clearlogo',
                     u'clearlogo': u'clearlogo',
                     u'hdclearart': u'clearart',
                     u'clearart': u'clearart',
                     u'tvthumb': u'landscape',
                     u'characterart': u'characterart',
                     u'seasonposter':u'seasonposters',
                     u'seasonbanner':u'seasonbanner',
                     u'seasonthumb': u'seasonlandscape'
                   }

try:
    source_params = dict(urlparse.parse_qsl(sys.argv[2]))
except IndexError:
    source_params = {}
source_settings = json.loads(source_params.get(u'pathSettings', u'{}'))

KEEPTITLE =source_settings.get(u'keeporiginaltitle', bool(ADDON.getSetting(u'keeporiginaltitle')))
CATLANDSCAPE = source_settings.get(u'cat_landscape', True)
VERBOSELOG =  source_settings.get(u'verboselog', bool(ADDON.getSetting(u'verboselog')))
LANG = source_settings.get(u'language', ADDON.getSetting(u'language'))
CERT_COUNTRY = source_settings.get(u'tmdbcertcountry', ADDON.getSetting(u'tmdbcertcountry')).lower()
IMAGEROOTURL, PREVIEWROOTURL = _load_base_urls()

if source_settings.get(u'usecertprefix', bool(ADDON.getSetting(u'usecertprefix'))):
    CERT_PREFIX = source_settings.get(u'certprefix', ADDON.getSetting(u'certprefix'))
else:
    CERT_PREFIX = u''
primary_rating = source_settings.get(u'ratings', ADDON.getSetting(u'ratings')).lower()
RATING_TYPES = [primary_rating]
if source_settings.get(u'imdbanyway', bool(ADDON.getSetting(u'imdbanyway'))) and primary_rating != u'imdb':
    RATING_TYPES.append(u'imdb')
if source_settings.get(u'traktanyway', bool(ADDON.getSetting(u'traktanyway'))) and primary_rating != u'trakt':
    RATING_TYPES.append(u'trakt')
if source_settings.get(u'tmdbanyway', bool(ADDON.getSetting(u'tmdbanyway'))) and primary_rating != u'tmdb':
    RATING_TYPES.append(u'tmdb')
FANARTTV_ENABLE = source_settings.get(u'enable_fanarttv', bool(ADDON.getSetting(u'enable_fanarttv')))
FANARTTV_CLIENTKEY = source_settings.get(u'fanarttv_clientkey', ADDON.getSetting(u'fanarttv_clientkey'))
