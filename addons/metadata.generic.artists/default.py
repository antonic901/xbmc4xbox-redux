# -*- coding: utf-8 -*-
from __future__ import absolute_import
import sys
from urlparse import parse_qsl
from lib.scraper import Scraper


class Main(object):
    def __init__(self):
        action, key, artist, url, nfo, settings = self._parse_argv()
        Scraper(action, key, artist, url, nfo, settings)

    def _parse_argv(self):
        params = dict(parse_qsl(sys.argv[2].lstrip(u'?')))
        # actions: find, resolveid, NfoUrl, getdetails
        action = params[u'action']
        # key: musicbrainz id
        key = params.get(u'key', u'')
        # artist: artistname
        artist = params.get(u'artist', u'')
        # url: provided by the scraper on previous run
        url = params.get(u'url', u'')
        # nfo: musicbrainz url from .nfo file
        nfo = params.get(u'nfo', u'')
        # path specific settings
        settings = params.get(u'pathSettings', {})
        return action, key, artist, url, nfo, settings


if (__name__ == u'__main__'):
    Main()
