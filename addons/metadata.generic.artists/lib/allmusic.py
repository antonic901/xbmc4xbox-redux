# -*- coding: utf-8 -*-
from __future__ import absolute_import
import difflib
import re

def allmusic_artistfind(data, artist):
    data = data.decode(u'utf-8')
    artists = []
    artistlist = re.findall(u'class="artist">\s*(.*?)\s*</li', data, re.S)
    for item in artistlist:
        artistdata = {}
        artistname = re.search(u'class="name">.*?>(.*?)</a', item, re.S)
        if artistname:
            artistdata[u'artist'] = artistname.group(1)
        else: # not likely to happen, but just in case
            continue
        # filter inaccurate results
        artistmatch = difflib.SequenceMatcher(None, artist.lower(), artistdata[u'artist'].lower()).ratio()
        if artistmatch > 0.95:
            artisturl = re.search(u'class="name">\s*<a href="(.*?)"', item)
            if artisturl:
                artistdata[u'url'] = artisturl.group(1)
            else: # not likely to happen, but just in case
                continue
            artists.append(artistdata)
            # we are only interested in the top result
            break
    return artists

def allmusic_artistdetails(data):
    data = data.decode(u'utf-8')
    artistdata = {}
    artist = re.search(ur'artist-name" itemprop="name">\s*(.*?)\s*<', data)
    if artist:
        artistdata[u'artist'] = artist.group(1)
    else:
        # no discography page available for this artist
        return
    active = re.search(ur'class="active-dates">.*?<div>(.*?)<', data, re.S)
    if active:
        artistdata[u'active'] = active.group(1)
    begin = re.search(ur'class="birth">.*?<h4>\s*(.*?)\s*<', data, re.S)
    if begin and begin.group(1) == u'Born':
        born = re.search(ur'class="birth">.*?<a.*?>(.*?)<', data, re.S)
        if born:
            artistdata[u'born'] = born.group(1)
    elif begin and begin.group(1) == u'Formed':
        formed = re.search(ur'class="birth">.*?<a.*?>(.*?)<', data, re.S)
        if formed:
            artistdata[u'formed'] = formed.group(1)
    end = re.search(ur'class="died">.*?<h4>\s*(.*?)\s*<', data, re.S)
    if end and end.group(1) == u'Died':
        died = re.search(ur'class="died">.*?<a.*?>(.*?)<', data, re.S)
        if died:
            artistdata[u'died'] = died.group(1)
    elif end and end.group(1) == u'Disbanded':
        disbanded = re.search(ur'class="died">.*?<a.*?>(.*?)<', data, re.S)
        if disbanded:
            artistdata[u'disbanded'] = disbanded.group(1)
    genre = re.search(ur'class="genre">.*?<a.*?>(.*?)<', data, re.S)
    if genre:
        artistdata[u'genre'] = genre.group(1)
    styledata = re.search(ur'class="styles">.*?<div>\s*(.*?)\s*</div', data, re.S)
    if styledata:
        styles = re.findall(ur'">(.*?)<', styledata.group(1))
        if styles:
            artistdata[u'styles'] = u' / '.join(styles)
    mooddata = re.search(ur'class="moods">.*?<li>\s*(.*?)\s*</ul', data, re.S)
    if mooddata:
        moods = re.findall(ur'">(.*?)<', mooddata.group(1))
        if moods:
            artistdata[u'moods'] = u' / '.join(moods)
    thumbsdata = re.search(ur'class="artist-image">.*?<img src="(.*?)"', data, re.S)
    if thumbsdata:
        thumbs = []
        thumbdata = {}
        thumb = thumbsdata.group(1).rstrip(u'?partner=allrovi.com')
        # 0=largest / 1=75 / 2=150 / 3=250 / 4=400 / 5=500 / 6=1080
        if thumb.endswith(u'f=4'):
            thumbdata[u'image'] = thumb.replace(u'f=4', u'f=0')
            thumbdata[u'preview'] = thumb.replace(u'f=4', u'f=2')
        else:
            thumbdata[u'image'] = thumb
            thumbdata[u'preview'] = thumb
        thumbdata[u'aspect'] = u'thumb'
        thumbs.append(thumbdata)
        artistdata[u'thumb'] = thumbs
    return artistdata

def allmusic_artistalbums(data):
    data = data.decode(u'utf-8')
    albums = []
    albumdata = re.search(ur'tbody>\s*(.*?)\s*</tbody', data, re.S)
    if albumdata:
        albumlist = re.findall(ur'tr.*?>\s*(.*?)\s*</tr', albumdata.group(1), re.S)
        if albumlist:
            for album in albumlist:
                albumdata = {}
                title = re.search(ur'<a.*?>(.*?)<', album)
                if title:
                    albumdata[u'title'] = title.group(1)
                year = re.search(ur'class="year".*?>\s*(.*?)\s*<', album)
                if year:
                    albumdata[u'year'] = year.group(1)
                else:
                    albumdata[u'year'] = u''
                if albumdata:
                    albums.append(albumdata)
    return albums
