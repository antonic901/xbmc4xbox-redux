# -*- coding: utf-8 -*-

from __future__ import absolute_import
import json
import socket
import sys
import time
import urllib2, urllib, urlparse
import urllib2, urllib
import _strptime # https://bugs.python.org/issue7980
from socket import timeout
from threading import Thread
from urllib2 import HTTPError, URLError
import xbmc
import xbmcaddon
import xbmcgui
import xbmcplugin
from .allmusic import allmusic_albumfind
from .allmusic import allmusic_albumdetails
from .discogs import discogs_albumfind
from .discogs import discogs_albummain
from .discogs import discogs_albumdetails
from .fanarttv import fanarttv_albumart
from .musicbrainz import musicbrainz_albumfind
from .musicbrainz import musicbrainz_albumdetails
from .musicbrainz import musicbrainz_albumlinks
from .musicbrainz import musicbrainz_albumart
from .nfo import nfo_geturl
from .theaudiodb import theaudiodb_albumdetails
from .wikipedia import wikipedia_albumdetails
from .utils import *

ADDONID = xbmcaddon.Addon().getAddonInfo(u'id')
ADDONNAME = xbmcaddon.Addon().getAddonInfo(u'name')
ADDONVERSION = xbmcaddon.Addon().getAddonInfo(u'version')


def log(txt):
    message = u'%s: %s' % (ADDONID, txt)
    xbmc.log(msg=message, level=xbmc.LOGDEBUG)

def get_data(url, jsonformat, retry=True):
    try:
        if url.startswith(u'https://musicbrainz.org/'):
            api_timeout(u'musicbrainztime')
        elif url.startswith(u'https://api.discogs.com/'):
            api_timeout(u'discogstime')
        headers = {}
        headers[u'User-Agent'] = u'%s/%s ( http://kodi.tv )' % (ADDONNAME, ADDONVERSION)
        req = urllib2.Request(url, headers=headers)
        resp = urllib2.urlopen(req, timeout=5)
        respdata = resp.read()
    except URLError, e:
        log(u'URLError: %s - %s' % (e.reason, url))
        return
    except HTTPError, e:
        log(u'HTTPError: %s - %s' % (e.reason, url))
        return
    except socket.timeout, e:
        log(u'socket: %s - %s' % (e, url))
        return
    if resp.getcode() == 503:
        log(u'exceeding musicbrainz api limit')
        if retry:
            xbmc.sleep(1000)
            get_data(url, jsonformat, retry=False)
        else:
            return
    elif resp.getcode() == 429:
        log(u'exceeding discogs api limit')
        if retry:
            xbmc.sleep(1000)
            get_data(url, jsonformat, retry=False)
        else:
            return
    if jsonformat:
        respdata = json.loads(respdata)
    return respdata

def api_timeout(scraper):
    currenttime = round(time.time() * 1000)
    previoustime = xbmcgui.Window(10000).getProperty(scraper)
    if previoustime:
        timeout = currenttime - int(previoustime)
        if timeout < 1000:
            xbmc.sleep(1000 - timeout)
    xbmcgui.Window(10000).setProperty(scraper, unicode(round(time.time() * 1000)))


class Scraper(object):
    def __init__(self, action, key, artist, album, url, nfo, settings):
        # parse path settings
        self.parse_settings(settings)
        # this is just for backward compitability with xml based scrapers https://github.com/xbmc/xbmc/pull/11632
        if action == u'resolveid':
            # return the result
            result = self.resolve_mbid(key)
            self.return_resolved(result)
        # search for artist name / album title matches
        elif action == u'find':
            # try musicbrainz first
            result = self.find_album(artist, album, u'musicbrainz')
            if result:
                self.return_search(result)
            # fallback to discogs
            else:
                result = self.find_album(artist, album, u'discogs')
                if result:
                    self.return_search(result)
        # return info id's
        elif action == u'getdetails':
            details = {}
            links = {}
            url = json.loads(url)
            artist = url.get(u'artist')
            album = url.get(u'album')
            mbalbumid = url.get(u'mbalbumid')
            mbreleasegroupid = url.get(u'mbreleasegroupid')
            dcid = url.get(u'dcalbumid')
            threads = []
            extrascrapers = []
            # we have musicbrainz album id
            if mbalbumid:
                # get the mbreleasegroupid, artist and album if we don't have them
                if not mbreleasegroupid:
                    result = self.get_details(mbalbumid, u'musicbrainz', details)
                    if not result:
                        scrapers = [[mbalbumid, u'musicbrainz']]
                    else:
                        mbreleasegroupid = details[u'musicbrainz'][u'mbreleasegroupid']
                        artist = details[u'musicbrainz'][u'artist_description']
                        album = details[u'musicbrainz'][u'album']
                        scrapers = [[mbreleasegroupid, u'theaudiodb'], [mbreleasegroupid, u'fanarttv'], [mbreleasegroupid, u'coverarchive']]
                else:
                    scrapers = [[mbalbumid, u'musicbrainz'], [mbreleasegroupid, u'theaudiodb'], [mbreleasegroupid, u'fanarttv'], [mbreleasegroupid, u'coverarchive']]
                # get musicbrainz links to other metadata sites
                lthread = Thread(target = self.get_links, args = (mbreleasegroupid, links))
                lthread.start()
                for item in scrapers:
                    thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                    threads.append(thread)
                    thread.start()
                # wait for the musicbrainz links to return
                lthread.join()
                if u'musicbrainz' in links:
                    # scrape allmusic if we have an url provided by musicbrainz
                    if u'allmusic' in links[u'musicbrainz']:
                        extrascrapers.append([{u'url': links[u'musicbrainz'][u'allmusic']}, u'allmusic'])
                    # only scrape allmusic by artistname and albumtitle if explicitly enabled
                    elif self.inaccurate and artist and album:
                        extrascrapers.append([{u'artist': artist, u'album': album}, u'allmusic'])
                    # scrape discogs if we have an url provided by musicbrainz
                    if u'discogs' in links[u'musicbrainz']:
                        extrascrapers.append([{u'masterurl': links[u'musicbrainz'][u'discogs']}, u'discogs'])
                    # only scrape discogs by artistname and albumtitle if explicitly enabled
                    elif self.inaccurate and artist and album:
                        extrascrapers.append([{u'artist': artist, u'album': album}, u'discogs'])
                    # scrape wikipedia if we have an url provided by musicbrainz
                    if u'wikipedia' in links[u'musicbrainz']:
                        extrascrapers.append([links[u'musicbrainz'][u'wikipedia'], u'wikipedia'])
                    elif u'wikidata' in links[u'musicbrainz']:
                        extrascrapers.append([links[u'musicbrainz'][u'wikidata'], u'wikidata'])
                for item in extrascrapers:
                    thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                    threads.append(thread)
                    thread.start()
            # we have a discogs id
            else:
                thread = Thread(target = self.get_details, args = ({u'url': dcid}, u'discogs', details))
                threads.append(thread)
                thread.start()
            for thread in threads:
                thread.join()
            result = self.compile_results(details)
            if result:
                self.return_details(result)
        # extract the mbalbumid from the provided musicbrainz url
        elif action == u'NfoUrl':
            # check if there is a musicbrainz url in the nfo file
            mbalbumid = nfo_geturl(nfo)
            if mbalbumid:
                # return the result
                result = self.resolve_mbid(mbalbumid)
                self.return_nfourl(result)
        xbmcplugin.endOfDirectory(int(sys.argv[1]))

    def parse_settings(self, data):
        settings = json.loads(data)
        # note: path settings are taken from the db, they may not reflect the current settings.xml file
        self.review = settings[u'review']
        self.genre = settings[u'genre']
        self.lang = settings[u'lang']
        self.mood = settings[u'mood']
        self.rating = settings[u'rating']
        self.style = settings[u'style']
        self.theme = settings[u'theme']
        self.inaccurate = settings[u'inaccurate']

    def resolve_mbid(self, mbalbumid):
        item = {}
        item[u'artist_description'] = u''
        item[u'album'] = u''
        item[u'mbalbumid'] = mbalbumid
        item[u'mbreleasegroupid'] = u''
        return item

    def find_album(self, artist, album, site):
        json = True
        # musicbrainz
        if site == u'musicbrainz':
            url = MUSICBRAINZURL % (MUSICBRAINZSEARCH % (urllib.quote_plus(album), urllib.quote_plus(artist), urllib.quote_plus(artist)))
            scraper = musicbrainz_albumfind
        # discogs
        elif site == u'discogs':
            url = DISCOGSURL % (DISCOGSSEARCH % (urllib.quote_plus(album), urllib.quote_plus(artist), DISCOGSKEY , DISCOGSSECRET))
            scraper = discogs_albumfind
        result = get_data(url, json)
        if not result:
            return
        albumresults = scraper(result, artist, album)
        return albumresults

    def get_links(self, param, links):
        json = True
        url = MUSICBRAINZURL % (MUSICBRAINZLINKS % param)
        result = get_data(url, json)
        if result:
            linkresults = musicbrainz_albumlinks(result)
            links[u'musicbrainz'] = linkresults
            return links

    def get_details(self, param, site, details):
        json = True
        # theaudiodb
        if site == u'theaudiodb':
            url = AUDIODBURL % (AUDIODBKEY, AUDIODBDETAILS % param)
            albumscraper = theaudiodb_albumdetails
        # musicbrainz
        elif site == u'musicbrainz':
            url = MUSICBRAINZURL % (MUSICBRAINZDETAILS % param)
            albumscraper = musicbrainz_albumdetails
        # fanarttv
        elif site == u'fanarttv':
            url = FANARTVURL % (param, FANARTVKEY)
            albumscraper = fanarttv_albumart
       # coverarchive
        elif site == u'coverarchive':
            url = MUSICBRAINZART % (param)
            albumscraper = musicbrainz_albumart
        # discogs
        elif site == u'discogs':
            # musicbrainz provides a link to the master release, but we need the main release
            if u'masterurl' in param:
                masterdata = get_data(DISCOGSURL % (DISCOGSMASTER % (param[u'masterurl'], DISCOGSKEY , DISCOGSSECRET)), True)
                if masterdata:
                    url = discogs_albummain(masterdata)
                    if url:
                        param[u'url'] = url
                    else:
                        return
                else:
                    return
            # search by artistname and albumtitle if we do not have an url
            if not u'url' in param:
                url = DISCOGSURL % (DISCOGSSEARCH % (urllib.quote_plus(param[u'album']), urllib.quote_plus(param[u'artist']), DISCOGSKEY , DISCOGSSECRET))
                albumresult = get_data(url, json)
                if albumresult:
                    albums = discogs_albumfind(albumresult, param[u'artist'], param[u'album'])
                    if albums:
                        albumresult = sorted(albums, key=lambda k: k[u'relevance'], reverse=True)
                        param[u'url'] = albumresult[0][u'dcalbumid']
                    else:
                        return
                else:
                    return
            url = DISCOGSURL % (DISCOGSDETAILS % (param[u'url'], DISCOGSKEY, DISCOGSSECRET))
            albumscraper = discogs_albumdetails
        # wikipedia
        elif site == u'wikipedia':
            url = WIKIPEDIAURL % param
            albumscraper = wikipedia_albumdetails
        elif site == u'wikidata':
            # resolve wikidata to wikipedia url
            result = get_data(WIKIDATAURL % param, json)
            try:
                album = result[u'entities'][param][u'sitelinks'][u'enwiki'][u'url'].rsplit(u'/', 1)[1]
            except:
                return
            site = u'wikipedia'
            url = WIKIPEDIAURL % album
            albumscraper = wikipedia_albumdetails
        # allmusic
        elif site == u'allmusic':
            json = False
            # search by artistname and albumtitle if we do not have an url
            if not u'url' in param:
                url = ALLMUSICURL % (ALLMUSICSEARCH % (urllib.quote_plus(param[u'artist']), urllib.quote_plus(param[u'album'])))
                albumresult = get_data(url, json)
                if albumresult:
                    albums = allmusic_albumfind(albumresult, param[u'artist'], param[u'album'])
                    if albums:
                        param[u'url'] = albums[0][u'url']
                    else:
                        return
                else:
                    return
            url = ALLMUSICDETAILS % param[u'url']
            albumscraper = allmusic_albumdetails
        result = get_data(url, json)
        if not result:
            return
        albumresults = albumscraper(result)
        if not albumresults:
            return
        details[site] = albumresults
        return details

    def compile_results(self, details):
        result = {}
        thumbs = []
        extras = []
        # merge metadata results, start with the least accurate sources
        if u'discogs' in details:
            for k, v in details[u'discogs'].items():
                if v:
                    result[k] = v
                if k == u'thumb' and v:
                    thumbs.append(v)
        if u'wikipedia' in details:
            for k, v in details[u'wikipedia'].items():
                if v:
                    result[k] = v
        if u'allmusic' in details:
            for k, v in details[u'allmusic'].items():
                if v:
                    result[k] = v
                if k == u'thumb' and v:
                    thumbs.append(v)
        if u'theaudiodb' in details:
            for k, v in details[u'theaudiodb'].items():
                if v:
                    result[k] = v
                if k == u'thumb' and v:
                    thumbs.append(v)
                if k == u'extras' and v:
                    extras.append(v)
        if u'musicbrainz' in details:
            for k, v in details[u'musicbrainz'].items():
                if v:
                    result[k] = v
        if u'coverarchive' in details:
            for k, v in details[u'coverarchive'].items():
                if v:
                    result[k] = v
                if k == u'thumb' and v:
                    thumbs.append(v)
                if k == u'extras' and v:
                    extras.append(v)
        # prefer artwork from fanarttv
        if u'fanarttv' in details:
            for k, v in details[u'fanarttv'].items():
                if v:
                    result[k] = v
                if k == u'thumb' and v:
                    thumbs.append(v)
                if k == u'extras' and v:
                    extras.append(v)
        # use musicbrainz artist as it provides the mbartistid (used for resolveid in the artist scraper)
        if u'musicbrainz' in details:
            result[u'artist'] = details[u'musicbrainz'][u'artist']
        # provide artwork from all scrapers for getthumb option
        if result:
            # thumb list from most accurate sources first
            thumbs.reverse()
            thumbnails = []
            for thumblist in thumbs:
                for item in thumblist:
                    thumbnails.append(item)
            # the order for extra art does not matter
            extraart = []
            for extralist in extras:
                for item in extralist:
                    extraart.append(item)
            # add the extra art to the end of the thumb list
            if extraart:
                thumbnails.extend(extraart)
            if thumbnails:
                result[u'thumb'] = thumbnails
        data = self.user_prefs(details, result)
        return data

    def user_prefs(self, details, result):
        # user preferences
        lang = u'description' + self.lang
        if self.review == u'theaudiodb' and u'theaudiodb' in details:
            if lang in details[u'theaudiodb']:
                result[u'description'] = details[u'theaudiodb'][lang]
            elif u'descriptionEN' in details[u'theaudiodb']:
                result[u'description'] = details[u'theaudiodb'][u'descriptionEN']
        elif (self.review in details) and (u'description' in details[self.review]):
            result[u'description'] = details[self.review][u'description']
        if (self.genre in details) and (u'genre' in details[self.genre]):
            result[u'genre'] = details[self.genre][u'genre']
        if (self.style in details) and (u'styles' in details[self.style]):
            result[u'styles'] = details[self.style][u'styles']
        if (self.mood in details) and (u'moods' in details[self.mood]):
            result[u'moods'] = details[self.mood][u'moods']
        if (self.theme in details) and (u'themes' in details[self.theme]):
            result[u'themes'] = details[self.theme][u'themes']
        if (self.rating in details) and (u'rating' in details[self.rating]):
            result[u'rating'] = details[self.rating][u'rating']
            result[u'votes'] = details[self.rating][u'votes']
        return result

    def return_search(self, data):
        items = []
        for item in data:
            listitem = xbmcgui.ListItem(item[u'album'], offscreen=True)
            listitem.setArt({u'thumb': item[u'thumb']})
            listitem.setProperty(u'album.artist', item[u'artist_description'])
            listitem.setProperty(u'album.year', item.get(u'year',u''))
            listitem.setProperty(u'album.type', item.get(u'type',u''))
            listitem.setProperty(u'album.releasestatus', item.get(u'releasestatus',u''))
            listitem.setProperty(u'album.label', item.get(u'label',u''))
            listitem.setProperty(u'relevance', item[u'relevance'])
            url = {u'artist':item[u'artist_description'], u'album':item[u'album']}
            if u'mbalbumid' in item:
                url[u'mbalbumid'] = item[u'mbalbumid']
                url[u'mbreleasegroupid'] = item[u'mbreleasegroupid']
            if u'dcalbumid' in item:
                url[u'dcalbumid'] = item[u'dcalbumid']
            items.append((json.dumps(url), listitem, True))
        if items:
            xbmcplugin.addDirectoryItems(handle=int(sys.argv[1]), items=items)

    def return_nfourl(self, item):
        listitem = xbmcgui.ListItem(offscreen=True)
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=json.dumps(item), listitem=listitem, isFolder=True)

    def return_resolved(self, item):
        listitem = xbmcgui.ListItem(path=json.dumps(item), offscreen=True)
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=listitem)

    def return_details(self, item):
        if not u'album' in item:
            return
        listitem = xbmcgui.ListItem(item[u'album'], offscreen=True)
        if u'mbalbumid' in item:
            listitem.setProperty(u'album.musicbrainzid', item[u'mbalbumid'])
            listitem.setProperty(u'album.releaseid', item[u'mbalbumid'])
        if u'mbreleasegroupid' in item:
            listitem.setProperty(u'album.releasegroupid', item[u'mbreleasegroupid'])
        if u'scrapedmbid' in item:
            listitem.setProperty(u'album.scrapedmbid', item[u'scrapedmbid'])
        if u'artist' in item:
            listitem.setProperty(u'album.artists', unicode(len(item[u'artist'])))
            for count, artist in enumerate(item[u'artist']):
                listitem.setProperty(u'album.artist%i.name' % (count + 1), artist[u'artist'])
                listitem.setProperty(u'album.artist%i.musicbrainzid' % (count + 1), artist.get(u'mbartistid', u''))
                listitem.setProperty(u'album.artist%i.sortname' % (count + 1), artist.get(u'artistsort', u''))
        if u'genre' in item:
            listitem.setProperty(u'album.genre', item[u'genre'])
        if u'styles' in item:
            listitem.setProperty(u'album.styles', item[u'styles'])
        if u'moods' in item:
            listitem.setProperty(u'album.moods', item[u'moods'])
        if u'themes' in item:
            listitem.setProperty(u'album.themes', item[u'themes'])
        if u'description' in item:
            listitem.setProperty(u'album.review', item[u'description'])
        if u'releasedate' in item:
            listitem.setProperty(u'album.releasedate', item[u'releasedate'])
        if u'originaldate' in item:
            listitem.setProperty(u'album.originaldate', item[u'originaldate'])
        if u'releasestatus' in item:
            listitem.setProperty(u'album.releasestatus', item[u'releasestatus'])
        if u'artist_description' in item:
            listitem.setProperty(u'album.artist_description', item[u'artist_description'])
        if u'label' in item:
            listitem.setProperty(u'album.label', item[u'label'])
        if u'type' in item:
            listitem.setProperty(u'album.type', item[u'type'])
        if u'compilation' in item:
            listitem.setProperty(u'album.compilation', item[u'compilation'])
        if u'year' in item:
            listitem.setProperty(u'album.year', item[u'year'])
        if u'rating' in item:
            listitem.setProperty(u'album.rating', item[u'rating'])
        if u'votes' in item:
            listitem.setProperty(u'album.votes', item[u'votes'])
        if u'thumb' in item:
            listitem.setProperty(u'album.thumbs', unicode(len(item[u'thumb'])))
            for count, thumb in enumerate(item[u'thumb']):
                listitem.setProperty(u'album.thumb%i.url' % (count + 1), thumb[u'image'])
                listitem.setProperty(u'album.thumb%i.aspect' % (count + 1), thumb[u'aspect'])
                listitem.setProperty(u'album.thumb%i.preview' % (count + 1), thumb[u'preview'])
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=listitem)
