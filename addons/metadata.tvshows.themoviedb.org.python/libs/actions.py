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

u"""Plugin route actions"""

from __future__ import absolute_import, unicode_literals

import sys
import urllib2, urllib, urlparse
import xbmcgui, xbmcplugin
from . import tmdb, data_utils
from .utils import logger, safe_get
try:
    from typing import Optional, Text, Union, ByteString  # pylint: disable=unused-import
except ImportError:
    pass

HANDLE = int(sys.argv[1])  # type: int


def find_show(title, year=None):
    # type: (Union[Text, bytes], Optional[Text]) -> None
    u"""Find a show by title"""
    if not isinstance(title, unicode):
        title = title.decode(u'utf-8')
    logger.debug(u'Searching for TV show {} ({})'.format(title, year))
    search_results = tmdb.search_show(title, year)
    for search_result in search_results:
        show_name = search_result[u'name']
        if safe_get(search_result, u'first_air_date') is not None:
            show_name += u' ({})'.format(search_result[u'first_air_date'][:4])
        list_item = xbmcgui.ListItem(show_name, offscreen=True)
        show_info = search_result
        list_item = data_utils.add_main_show_info(list_item, show_info, full_info=False)
        # Below "url" is some unique ID string (may be an actual URL to a show page)
        # that is used to get information about a specific TV show.
        xbmcplugin.addDirectoryItem(
            HANDLE,
            url=unicode(search_result[u'id']),
            listitem=list_item,
            isFolder=True
        )


def get_show_id_from_nfo(nfo):
    # type: (Text) -> None
    u"""
    Get show ID by NFO file contents

    This function is called first instead of find_show
    if a NFO file is found in a TV show folder.

    :param nfo: the contents of a NFO file
    """
    if isinstance(nfo, str):
        nfo = nfo.decode(u'utf-8', u'replace')
    logger.debug(u'Parsing NFO file:\n{}'.format(nfo))
    parse_result, named_seasons = data_utils.parse_nfo_url(nfo)
    if parse_result:
        if parse_result.provider == u'themoviedb':
            show_info = tmdb.load_show_info(parse_result.show_id, ep_grouping=parse_result.ep_grouping, named_seasons=named_seasons)
        else:
            show_info = None
        if show_info is not None:
            list_item = xbmcgui.ListItem(show_info[u'name'], offscreen=True)
            # "url" is some string that unique identifies a show.
            # It may be an actual URL of a TV show page.
            xbmcplugin.addDirectoryItem(
                HANDLE,
                url=unicode(show_info[u'id']),
                listitem=list_item,
                isFolder=True
            )


def get_details(show_id):
    # type: (Text) -> None
    u"""Get details about a specific show"""
    logger.debug(u'Getting details for show id {}'.format(show_id))
    show_info = tmdb.load_show_info(show_id)
    if show_info is not None:
        list_item = xbmcgui.ListItem(show_info[u'name'], offscreen=True)
        list_item = data_utils.add_main_show_info(list_item, show_info, full_info=True)
        xbmcplugin.setResolvedUrl(HANDLE, True, list_item)
    else:
        xbmcplugin.setResolvedUrl(HANDLE, False, xbmcgui.ListItem(offscreen=True))


def get_episode_list(show_id):  # pylint: disable=missing-docstring
    # type: (Text) -> None
    logger.debug(u'Getting episode list for show id {}'.format(show_id))
    if not show_id.isdigit():
        # Kodi has a bug: when a show directory contains an XML NFO file with
        # episodeguide URL, that URL is always passed here regardless of
        # the actual parsing result in get_show_from_nfo()
        parse_result, named_seasons = data_utils.parse_nfo_url(show_id)
        if not parse_result:
            return
        if parse_result.provider == u'themoviedb' or parse_result.provider == u'tmdb':
            show_info = tmdb.load_show_info(parse_result.show_id)
        else:
            return
    else:
        show_info = tmdb.load_show_info(show_id)
    if show_info is not None:
        theindex = 0
        for episode in show_info[u'episodes']:
            epname = episode.get(u'name', u'Episode ' + unicode(episode[u'episode_number']))
            list_item = xbmcgui.ListItem(epname, offscreen=True)
            list_item = data_utils.add_episode_info(list_item, episode, full_info=False)
            encoded_ids = urllib.urlencode(
                {u'show_id': unicode(show_info[u'id']), u'episode_id': unicode(theindex)}
            )
            theindex = theindex + 1
            # Below "url" is some unique ID string (may be an actual URL to an episode page)
            # that allows to retrieve information about a specific episode.
            url = urllib.quote(encoded_ids)
            xbmcplugin.addDirectoryItem(
                HANDLE,
                url=url,
                listitem=list_item,
                isFolder=True
            )


def get_episode_details(encoded_ids):  # pylint: disable=missing-docstring
    # type: (Text) -> None
    encoded_ids = urllib.unquote(encoded_ids)
    decoded_ids = dict(urlparse.parse_qsl(encoded_ids))
    logger.debug(u'Getting episode details for {}'.format(decoded_ids))
    episode_info = tmdb.load_episode_info(
        decoded_ids[u'show_id'], decoded_ids[u'episode_id']
    )
    if episode_info:
        list_item = xbmcgui.ListItem(episode_info[u'name'], offscreen=True)
        list_item = data_utils.add_episode_info(list_item, episode_info, full_info=True)
        xbmcplugin.setResolvedUrl(HANDLE, True, list_item)
    else:
        xbmcplugin.setResolvedUrl(HANDLE, False, xbmcgui.ListItem(offscreen=True))


def get_artwork(show_id):
    # type: (Text) -> None
    u"""
    Get available artwork for a show

    :param show_id: default unique ID set by setUniqueIDs() method
    """
    if not show_id:
      return
    logger.debug(u'Getting artwork for show ID {}'.format(show_id))
    show_info = tmdb.load_show_info(show_id)
    if show_info is not None:
        list_item = xbmcgui.ListItem(show_info[u'name'], offscreen=True)
        list_item = data_utils.set_show_artwork(show_info, list_item)
        xbmcplugin.setResolvedUrl(HANDLE, True, list_item)
    else:
        xbmcplugin.setResolvedUrl(HANDLE, False, xbmcgui.ListItem(offscreen=True))


def router(paramstring):
    # type: (Text) -> None
    u"""
    Route addon calls

    :param paramstring: url-encoded query string
    :raises RuntimeError: on unknown call action
    """
    params = dict(urlparse.parse_qsl(paramstring))
    logger.debug(u'Called addon with params: {}'.format(sys.argv))
    if params[u'action'] == u'find':
        logger.debug(u'performing find action')
        find_show(params[u'title'], params.get(u'year'))
    elif params[u'action'].lower() == u'nfourl':
        logger.debug(u'performing nfourl action')
        get_show_id_from_nfo(params[u'nfo'])
    elif params[u'action'] == u'getdetails':
        logger.debug(u'performing getdetails action')
        get_details(params[u'url'])
    elif params[u'action'] == u'getepisodelist':
        logger.debug(u'performing getepisodelist action')
        get_episode_list(params[u'url'])
    elif params[u'action'] == u'getepisodedetails':
        logger.debug(u'performing getepisodedetails action')
        get_episode_details(params[u'url'])
    elif params[u'action'] == u'getartwork':
        logger.debug(u'performing getartwork action')
        get_artwork(params.get(u'id'))
    else:
        raise RuntimeError(u'Invalid addon call: {}'.format(sys.argv))
    xbmcplugin.endOfDirectory(HANDLE)
