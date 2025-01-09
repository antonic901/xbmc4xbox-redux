# -*- coding: utf-8 -*-
from __future__ import absolute_import
import difflib

def discogs_artistfind(data, artist):
    artists = []
    for item in data.get(u'results',[]):
        artistdata = {}
        artistdata[u'artist'] = item[u'title']
        # filter inaccurate results
        match = difflib.SequenceMatcher(None, artist.lower(), item[u'title'].lower()).ratio()
        score = round(match, 2)
        if score > 0.90:
            artistdata[u'thumb'] = item[u'thumb']
            artistdata[u'genre'] = u''
            artistdata[u'born'] = u''
            artistdata[u'dcid'] = item[u'id']
            # discogs does not provide relevance, use our own
            artistdata[u'relevance'] = unicode(score)
            artists.append(artistdata)
    return artists

def discogs_artistdetails(data):
    artistdata = {}
    artistdata[u'artist'] = data[u'name']
    artistdata[u'biography'] = data[u'profile']
    if u'images' in data:
        thumbs = []
        for item in data[u'images']:
            thumbdata = {}
            thumbdata[u'image'] = item[u'uri']
            thumbdata[u'preview'] = item[u'uri150']
            thumbdata[u'aspect'] = u'thumb'
            thumbs.append(thumbdata)
        artistdata[u'thumb'] = thumbs
    return artistdata

def discogs_artistalbums(data):
    albums = []
    for item in data[u'releases']:
        if item[u'role'] == u'Main':
            albumdata = {}
            albumdata[u'title'] = item[u'title']
            albumdata[u'year'] = unicode(item.get(u'year', u''))
            albums.append(albumdata)
    return albums
