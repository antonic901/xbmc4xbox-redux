# -*- coding: utf-8 -*-
from __future__ import absolute_import
import re

def wikipedia_albumdetails(data):
    albumdata = {}
    # check in case musicbrainz did not provide a direct link
    if u'extract' in data[u'query'][u'pages'][0] and not data[u'query'][u'pages'][0][u'extract'].endswith(u'may refer to:'):
        albumdata[u'description'] = re.sub(u'\n\n\n== .*? ==\n', u' ', data[u'query'][u'pages'][0][u'extract'])
    return albumdata
