# -*- coding: utf-8 -*-

from __future__ import absolute_import
import re

def nfo_geturl(data):
    result = re.search(u'https://musicbrainz.org/(ws/2/)?release/([0-9a-z\-]*)', data)
    if result:
        return result.group(2)
