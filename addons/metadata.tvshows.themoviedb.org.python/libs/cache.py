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

u"""Cache-related functionality"""

from __future__ import with_statement
from __future__ import absolute_import, unicode_literals

import os, pickle
import xbmc, xbmcvfs

from .utils import ADDON, logger
from io import open

try:
    from typing import Optional, Text, Dict, Any  # pylint: disable=unused-import
except ImportError:
    pass



def _get_cache_directory():  # pylint: disable=missing-docstring
    # type: () -> Text
    temp_dir = xbmc.translatePath(u'special://temp')
    cache_dir = os.path.join(temp_dir, u'scrapers', ADDON.getAddonInfo(u'id'))
    if not xbmcvfs.exists(cache_dir):
        xbmcvfs.mkdir(cache_dir)
    logger.debug(u'the cache dir is ' + cache_dir)
    return cache_dir


CACHE_DIR = _get_cache_directory()  # type: Text


def cache_show_info(show_info):
    # type: (Dict[Text, Any]) -> None
    u"""
    Save show_info dict to cache
    """
    file_name = unicode(show_info[u'id']) + u'.pickle'
    cache = {
        u'show_info': show_info
    }
    with open(os.path.join(CACHE_DIR, file_name), u'wb') as fo:
        pickle.dump(cache, fo, protocol=2)


def load_show_info_from_cache(show_id):
    # type: (Text) -> Optional[Dict[Text, Any]]
    u"""
    Load show info from a local cache

    :param show_id: show ID on TVmaze
    :return: show_info dict or None
    """
    file_name = unicode(show_id) + u'.pickle'
    try:
        with open(os.path.join(CACHE_DIR, file_name), u'rb') as fo:
            load_kwargs = {}
            load_kwargs[u'encoding'] = u'bytes'
            cache = pickle.load(fo, **load_kwargs)
        return cache[u'show_info']
    except (IOError, pickle.PickleError), exc:
        logger.debug(u'Cache message: {} {}'.format(type(exc), exc))
        return None
