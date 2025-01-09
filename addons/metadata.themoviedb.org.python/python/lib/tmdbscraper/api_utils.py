# coding: utf-8
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

u"""Functions to interact with various web site APIs."""

from __future__ import absolute_import, unicode_literals

import json, xbmc
# from pprint import pformat
try: #PY2 / PY3
    from urllib2 import Request, urlopen
    from urllib2 import URLError
    from urllib import urlencode
except ImportError:
    from urllib2 import Request, urlopen
from urllib2 import URLError
from urllib import urlencode
try:
    from typing import Text, Optional, Union, List, Dict, Any  # pylint: disable=unused-import
    InfoType = Dict[Text, Any]  # pylint: disable=invalid-name
except ImportError:
    pass

HEADERS = {}


def set_headers(headers):
    HEADERS.update(headers)


def load_info(url, params=None, default=None, resp_type = u'json'):
    # type: (Text, Optional[Dict[Text, Union[Text, List[Text]]]]) -> Union[dict, list]
    u"""
    Load info from external api

    :param url: API endpoint URL
    :param params: URL query params
    :default: object to return if there is an error
    :resp_type: what to return to the calling function
    :return: API response or default on error
    """
    theerror = u''
    if params:
        url = url + u'?' + urlencode(params)
    xbmc.log(u'Calling URL "{}"'.format(url), xbmc.LOGDEBUG)
    req = Request(url, headers=HEADERS)
    try:
        response = urlopen(req)
    except URLError, e:
        if hasattr(e, u'reason'):
            theerror = {u'error': u'failed to reach the remote site\nReason: {}'.format(e.reason)}
        elif hasattr(e, u'code'):
            theerror = {u'error': u'remote site unable to fulfill the request\nError code: {}'.format(e.code)}
        if default is not None:
            return default
        else:
            return theerror
    if resp_type.lower() == u'json':
        resp = json.loads(response.read().decode(u'utf-8'))
    else:
        resp = response.read().decode(u'utf-8')
    # xbmc.log('the api response:\n{}'.format(pformat(resp)), xbmc.LOGDEBUG)
    return resp
