# -*- coding: utf-8 -*-

from __future__ import absolute_import
import datetime
import difflib
import time
import re

def allmusic_albumfind(data, artist, album):
    data = data.decode(u'utf-8')
    albums = []
    albumlist = re.findall(u'class="album">\s*(.*?)\s*</li', data, re.S)
    for item in albumlist:
        albumdata = {}
        albumartist = re.search(u'class="artist">.*?>(.*?)</a', item, re.S)
        if albumartist:
            albumdata[u'artist'] = albumartist.group(1)
        else: # classical album
            continue
        albumname = re.search(u'class="title">.*?>(.*?)</a', item, re.S)
        if albumname:
            albumdata[u'album'] = albumname.group(1)
        else: # not likely to happen, but just in case
            continue
        # filter inaccurate results
        artistmatch = difflib.SequenceMatcher(None, artist.lower(), albumdata[u'artist'].lower()).ratio()
        albummatch = difflib.SequenceMatcher(None, album.lower(), albumdata[u'album'].lower()).ratio()
        if artistmatch > 0.90 and albummatch > 0.90:
            albumurl = re.search(u'class="title">\s*<a href="(.*?)"', item)
            if albumurl:
                albumdata[u'url'] = albumurl.group(1)
            else: # not likely to happen, but just in case
                continue
            albums.append(albumdata)
            # we are only interested in the top result
            break
    return albums

def allmusic_albumdetails(data):
    data = data.decode(u'utf-8')
    albumdata = {}
    releasedata = re.search(u'class="release-date">.*?<span>(.*?)<', data, re.S)
    if releasedata:
        dateformat = releasedata.group(1)
        if len(dateformat) > 4:
            try:
                # month day, year
                albumdata[u'releasedate'] = datetime.datetime(*(time.strptime(dateformat, u'%B %d, %Y')[0:3])).strftime(u'%Y-%m-%d')
            except:
                # month, year
                albumdata[u'releasedate'] = datetime.datetime(*(time.strptime(dateformat, u'%B, %Y')[0:3])).strftime(u'%Y-%m')
        else:
            # year
            albumdata[u'releasedate'] = dateformat
    yeardata = re.search(u'class="year".*?>\s*(.*?)\s*<', data)
    if yeardata:
        albumdata[u'year'] = yeardata.group(1)
    genredata = re.search(u'class="genre">.*?">(.*?)<', data, re.S)
    if genredata:
        albumdata[u'genre'] = genredata.group(1)
    styledata = re.search(u'class="styles">.*?div>\s*(.*?)\s*</div', data, re.S)
    if styledata:
        stylelist = re.findall(u'">(.*?)<', styledata.group(1))
        if stylelist:
            albumdata[u'styles'] =  u' / '.join(stylelist)
    mooddata = re.search(u'class="moods">.*?div>\s*(.*?)\s*</div', data, re.S)
    if mooddata:
        moodlist = re.findall(u'">(.*?)<', mooddata.group(1))
        if moodlist:
            albumdata[u'moods'] =  u' / '.join(moodlist)
    themedata = re.search(u'class="themes">.*?div>\s*(.*?)\s*</div', data, re.S)
    if themedata:
        themelist = re.findall(u'">(.*?)<', themedata.group(1))
        if themelist:
            albumdata[u'themes'] =  u' / '.join(themelist)
    ratingdata = re.search(u'itemprop="ratingValue">\s*(.*?)\s*</div', data)
    if ratingdata:
        albumdata[u'rating'] = ratingdata.group(1)
    albumdata[u'votes'] = u''
    titledata = re.search(u'class="album-title".*?>\s*(.*?)\s*<', data, re.S)
    if titledata:
        albumdata[u'album'] = titledata.group(1)
    labeldata = re.search(u'class="label-catalog".*?<.*?>(.*?)<', data, re.S)
    if labeldata:
        albumdata[u'label'] = labeldata.group(1)
    artistdata = re.search(u'class="album-artist".*?<span.*?>\s*(.*?)\s*</span', data, re.S)
    if artistdata:
        artistlist = re.findall(u'">(.*?)<', artistdata.group(1))
        artists = []
        for item in artistlist:
            artistinfo = {}
            artistinfo[u'artist'] = item
            artists.append(artistinfo)
        if artists:
            albumdata[u'artist'] = artists
            albumdata[u'artist_description'] = u' / '.join(artistlist)
    thumbsdata = re.search(u'class="album-contain".*?src="(.*?)"', data, re.S)
    if thumbsdata:
        thumbs = []
        thumbdata = {}
        thumb = thumbsdata.group(1).rstrip(u'?partner=allrovi.com')
        # ignore internal blank thumb
        if thumb.startswith(u'http'):
            # 0=largest / 1=75 / 2=150 / 3=250 / 4=400 / 5=500 / 6=1080
            if thumb.endswith(u'f=5'):
                thumbdata[u'image'] = thumb.replace(u'f=5', u'f=0')
                thumbdata[u'preview'] = thumb.replace(u'f=5', u'f=2')
            else:
                thumbdata[u'image'] = thumb
                thumbdata[u'preview'] = thumb
            thumbdata[u'aspect'] = u'thumb'
            thumbs.append(thumbdata)
            albumdata[u'thumb'] = thumbs
    return albumdata
