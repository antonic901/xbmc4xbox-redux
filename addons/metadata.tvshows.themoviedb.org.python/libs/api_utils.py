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

u"""Functions to interact with various web site APIs"""

from __future__ import absolute_import, unicode_literals

import json
from urllib2 import Request, urlopen
from urllib2 import URLError
from urllib import urlencode
from pprint import pformat
from .utils import logger
try:
    from typing import Text, Optional, Union, List, Dict, Any  # pylint: disable=unused-import
    InfoType = Dict[Text, Any]  # pylint: disable=invalid-name
except ImportError:
    pass

HEADERS = {}


def set_headers(headers):
    HEADERS.update(headers)


def load_info(url, params=None, default=None, resp_type = u'json', verboselog=False):
    # type: (Text, Optional[Dict[Text, Union[Text, List[Text]]]]) -> Union[dict, list]
    u"""
    Load info from external api

    :param url: API endpoint URL
    :param params: URL query params
    :default: object to return if there is an error
    :resp_type: what to return to the calling function
    :return: API response or default on error
    """
    if params:
        url = url + u'?' + urlencode(params)
    logger.debug(u'Calling URL "{}"'.format(url))
    req = Request(url, headers=HEADERS)
    try:
        response = urlopen(req)
    except URLError, e:
        if hasattr(e, u'reason'):
            logger.debug(u'failed to reach the remote site\nReason: {}'.format(e.reason))
        elif hasattr(e, u'code'):
            logger.debug(u'remote site unable to fulfill the request\nError code: {}'.format(e.code))
        response = None
    if response is None:
        resp = default
    elif resp_type.lower() == u'json':
        resp = json.loads(response.read().decode(u'utf-8'))
    else:
        resp = response.read().decode(u'utf-8')
    if verboselog:
        logger.debug(u'the api response:\n{}'.format(pformat(resp)))
    return resp
