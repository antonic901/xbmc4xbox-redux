# -*- coding: utf-8 -*-

def fanarttv_artistart(data):
    artistdata = {}
    extras = []
    if u'artistbackground' in data:
        fanart = []
        for item in data[u'artistbackground']:
            fanartdata = {}
            fanartdata[u'image'] = item[u'url']
            fanartdata[u'preview'] = item[u'url'].replace(u'/fanart/', u'/preview/')
            fanartdata[u'aspect'] = u'fanart'
            fanart.append(fanartdata)
        artistdata[u'fanart'] = fanart
    if u'artistthumb' in data:
        thumbs = []
        for item in data[u'artistthumb']:
            thumbdata = {}
            thumbdata[u'image'] = item[u'url']
            thumbdata[u'preview'] = item[u'url'].replace(u'/fanart/', u'/preview/')
            thumbdata[u'aspect'] = u'thumb'
            thumbs.append(thumbdata)
        if thumbs:
            artistdata[u'thumb'] = thumbs
    if u'musicbanner' in data:
        for item in data[u'musicbanner']:
            extradata = {}
            extradata[u'image'] = item[u'url']
            extradata[u'preview'] = item[u'url'].replace(u'/fanart/', u'/preview/')
            extradata[u'aspect'] = u'banner'
            extras.append(extradata)
    if u'hdmusiclogo' in data:
        for item in data[u'hdmusiclogo']:
            extradata = {}
            extradata[u'image'] = item[u'url']
            extradata[u'preview'] = item[u'url'].replace(u'/fanart/', u'/preview/')
            extradata[u'aspect'] = u'clearlogo'
            extras.append(extradata)
    elif u'musiclogo' in data:
        for item in data[u'musiclogo']:
            extradata = {}
            extradata[u'image'] = item[u'url']
            extradata[u'preview'] = item[u'url'].replace(u'/fanart/', u'/preview/')
            extradata[u'aspect'] = u'clearlogo'
            extras.append(extradata)
    if extras:
        artistdata[u'extras'] = extras
    return artistdata
