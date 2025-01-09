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
from .allmusic import allmusic_artistfind
from .allmusic import allmusic_artistdetails
from .allmusic import allmusic_artistalbums
from .discogs import discogs_artistfind
from .discogs import discogs_artistdetails
from .discogs import discogs_artistalbums
from .fanarttv import fanarttv_artistart
from .musicbrainz import musicbrainz_artistfind
from .musicbrainz import musicbrainz_artistdetails
from .nfo import nfo_geturl
from .theaudiodb import theaudiodb_artistdetails
from .theaudiodb import theaudiodb_artistalbums
from .wikipedia import wikipedia_artistdetails
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
    def __init__(self, action, key, artist, url, nfo, settings):
        # parse path settings
        self.parse_settings(settings)
        # this is just for backward compitability with xml based scrapers https://github.com/xbmc/xbmc/pull/11632
        if action == u'resolveid':
            # return the result
            result = self.resolve_mbid(key)
            self.return_resolved(result)
        # search for artist name matches
        elif action == u'find':
            # try musicbrainz first
            result = self.find_artist(artist, u'musicbrainz')
            if result:
                self.return_search(result)
            # fallback to discogs
            else:
                result = self.find_artist(artist, u'discogs')
                if result:
                    self.return_search(result)
        # return info using id's
        elif action == u'getdetails':
            details = {}
            discography = {}
            url = json.loads(url)
            artist = url.get(u'artist')
            mbartistid = url.get(u'mbartistid')
            dcid = url.get(u'dcid')
            threads = []
            extrascrapers = []
            discographyscrapers = []
            # we have a musicbrainz id
            if mbartistid:
                scrapers = [[mbartistid, u'musicbrainz'], [mbartistid, u'theaudiodb'], [mbartistid, u'fanarttv']]
                for item in scrapers:
                    thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                    threads.append(thread)
                    thread.start()
                # theaudiodb discograhy
                thread = Thread(target = self.get_discography, args = (mbartistid, u'theaudiodb', discography))
                threads.append(thread)
                thread.start()
                # wait for musicbrainz to finish
                threads[0].join()
                # check if we have a result:
                if u'musicbrainz' in details:
                    if not artist:
                        artist = details[u'musicbrainz'][u'artist']
                    # scrape allmusic if we have an url provided by musicbrainz
                    if u'allmusic' in details[u'musicbrainz']:
                        extrascrapers.append([{u'url': details[u'musicbrainz'][u'allmusic']}, u'allmusic'])
                        # allmusic discograhy
                        discographyscrapers.append([{u'url': details[u'musicbrainz'][u'allmusic']}, u'allmusic'])
                    # only scrape allmusic by artistname if explicitly enabled
                    elif self.inaccurate and artist:
                        extrascrapers.append([{u'artist': artist}, u'allmusic'])
                    # scrape wikipedia if we have an url provided by musicbrainz
                    if u'wikipedia' in details[u'musicbrainz']:
                        extrascrapers.append([details[u'musicbrainz'][u'wikipedia'], u'wikipedia'])
                    elif u'wikidata' in details[u'musicbrainz']:
                        extrascrapers.append([details[u'musicbrainz'][u'wikidata'], u'wikidata'])
                    # scrape discogs if we have an url provided by musicbrainz
                    if u'discogs' in details[u'musicbrainz']:
                        extrascrapers.append([{u'url': details[u'musicbrainz'][u'discogs']}, u'discogs'])
                        # discogs discograhy
                        discographyscrapers.append([{u'url': details[u'musicbrainz'][u'discogs']}, u'discogs'])
                    # only scrape discogs by artistname if explicitly enabled
                    elif self.inaccurate and artist:
                        extrascrapers.append([{u'artist': artist}, u'discogs'])
                    for item in extrascrapers:
                        thread = Thread(target = self.get_details, args = (item[0], item[1], details))
                        threads.append(thread)
                        thread.start()
                    # get allmusic / discogs discography if we have an url
                    for item in discographyscrapers:
                        thread = Thread(target = self.get_discography, args = (item[0], item[1], discography))
                        threads.append(thread)
                        thread.start()
            # we have a discogs id
            else:
                thread = Thread(target = self.get_details, args = ({u'url': dcid}, u'discogs', details))
                threads.append(thread)
                thread.start()
                thread = Thread(target = self.get_discography, args = ({u'url': dcid}, u'discogs', discography))
                threads.append(thread)
                thread.start()
            if threads:
                for thread in threads:
                    thread.join()
            # merge discography items
            for site, albumlist in discography.items():
                if site in details:
                    details[site][u'albums'] = albumlist
                else:
                    details[site] = {}
                    details[site][u'albums'] = albumlist
            result = self.compile_results(details)
            if result:
                self.return_details(result)
        elif action == u'NfoUrl':
            # check if there is a musicbrainz url in the nfo file
            mbartistid = nfo_geturl(nfo)
            if mbartistid:
                # return the result
                result = self.resolve_mbid(mbartistid)
                self.return_nfourl(result)
        xbmcplugin.endOfDirectory(int(sys.argv[1]))

    def parse_settings(self, data):
        settings = json.loads(data)
        # note: path settings are taken from the db, they may not reflect the current settings.xml file
        self.bio = settings[u'bio']
        self.discog = settings[u'discog']
        self.genre = settings[u'genre']
        self.lang = settings[u'lang']
        self.mood = settings[u'mood']
        self.style = settings[u'style']
        self.inaccurate = settings[u'inaccurate']

    def resolve_mbid(self, mbartistid):
        item = {}
        item[u'artist'] = u''
        item[u'mbartistid'] = mbartistid
        return item

    def find_artist(self, artist, site):
        json = True
        # musicbrainz
        if site == u'musicbrainz':
            url = MUSICBRAINZURL % (MUSICBRAINZSEARCH % urllib.quote_plus(artist))
            scraper = musicbrainz_artistfind
        # musicbrainz
        if site == u'discogs':
            url = DISCOGSURL % (DISCOGSSEARCH % (urllib.quote_plus(artist), DISCOGSKEY , DISCOGSSECRET))
            scraper = discogs_artistfind
        result = get_data(url, json)
        if not result:
            return
        artistresults = scraper(result, artist)
        return artistresults

    def get_details(self, param, site, details, discography={}):
        json = True
        # theaudiodb
        if site == u'theaudiodb':
            url = AUDIODBURL % (AUDIODBKEY, AUDIODBDETAILS % param)
            artistscraper = theaudiodb_artistdetails
        # musicbrainz
        elif site == u'musicbrainz':
            url = MUSICBRAINZURL % (MUSICBRAINZDETAILS % param)
            artistscraper = musicbrainz_artistdetails
        # fanarttv
        elif site == u'fanarttv':
            url = FANARTVURL % (param, FANARTVKEY)
            artistscraper = fanarttv_artistart
        # discogs
        elif site == u'discogs':
            # search by artistname if we do not have an url
            if u'artist' in param:
                url = DISCOGSURL % (DISCOGSSEARCH % (urllib.quote_plus(param[u'artist']), DISCOGSKEY , DISCOGSSECRET))
                artistresult = get_data(url, json)
                if artistresult:
                    artists = discogs_artistfind(artistresult, param[u'artist'])
                    if artists:
                        artistresult = sorted(artists, key=lambda k: k[u'relevance'], reverse=True)
                        param[u'url'] = artistresult[0][u'dcid']
                    else:
                        return
                else:
                    return
            url = DISCOGSURL % (DISCOGSDETAILS % (param[u'url'], DISCOGSKEY, DISCOGSSECRET))
            artistscraper = discogs_artistdetails
        # wikipedia
        elif site == u'wikipedia':
            url = WIKIPEDIAURL % param
            artistscraper = wikipedia_artistdetails
        elif site == u'wikidata':
            # resolve wikidata to wikipedia url
            result = get_data(WIKIDATAURL % param, json)
            try:
                artist = result[u'entities'][param][u'sitelinks'][u'enwiki'][u'url'].rsplit(u'/', 1)[1]
            except:
                return
            site = u'wikipedia'
            url = WIKIPEDIAURL % artist
            artistscraper = wikipedia_artistdetails
        # allmusic
        elif site == u'allmusic':
            json = False
            # search by artistname if we do not have an url
            if u'artist' in param:
                url = ALLMUSICURL % urllib.quote_plus(param[u'artist'])
                artistresult = get_data(url, json)
                if artistresult:
                    artists = allmusic_artistfind(artistresult, param[u'artist'])
                    if artists:
                        param[u'url'] = artists[0][u'url']
                    else:
                        return
                else:
                    return
            url = param[u'url']
            artistscraper = allmusic_artistdetails
        result = get_data(url, json)
        if not result:
            return
        artistresults = artistscraper(result)
        if not artistresults:
            return
        details[site] = artistresults
        # get allmusic / discogs discography if we searched by artistname
        if (site == u'discogs' or site == u'allmusic') and u'artist' in param:
            albums = self.get_discography(param, site, {})
            if albums:
                details[site][u'albums'] = albums[site]
        return details

    def get_discography(self, param, site, discography):
        json = True
        if site == u'theaudiodb':
            # theaudiodb - discography
            albumsurl = AUDIODBURL % (AUDIODBKEY, AUDIODBDISCOGRAPHY % param)
            scraper = theaudiodb_artistalbums
        elif site == u'discogs':
            # discogs - discography
            albumsurl = DISCOGSURL % (DISCOGSDISCOGRAPHY % (param[u'url'], DISCOGSKEY, DISCOGSSECRET))
            scraper = discogs_artistalbums
        elif site == u'allmusic':
            # allmusic - discography
            json = False
            albumsurl = param[u'url'] + u'/discography'
            scraper = allmusic_artistalbums
        albumdata = get_data(albumsurl, json)
        if not albumdata:
            return
        albumresults = scraper(albumdata)
        if not albumresults:
            return
        discography[site] = albumresults
        return discography

    def compile_results(self, details):
        result = {}
        thumbs = []
        fanart = []
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
                elif k == u'fanart' and v:
                    fanart.append(v)
                if k == u'extras' and v:
                    extras.append(v)
        if u'musicbrainz' in details:
            for k, v in details[u'musicbrainz'].items():
                if v:
                    result[k] = v
        if u'fanarttv' in details:
            for k, v in details[u'fanarttv'].items():
                if v:
                    result[k] = v
                if k == u'thumb' and v:
                    thumbs.append(v)
                elif k == u'fanart' and v:
                    fanart.append(v)
                if k == u'extras' and v:
                    extras.append(v)
        # merge artwork from all scrapers
        if result:
            # artworks from most accurate sources first
            thumbs.reverse()
            thumbnails = []
            fanart.reverse()
            fanarts = []
            # the order for extra art does not matter
            extraart = []
            for thumblist in thumbs:
                for item in thumblist:
                    thumbnails.append(item)
            for extralist in extras:
                for item in extralist:
                    extraart.append(item)
            # add the extra art to the end of the thumb list
            if extraart:
                thumbnails.extend(extraart)
            for fanartlist in fanart:
                for item in fanartlist:
                    fanarts.append(item)
            # add the fanart to the end of the thumb list
            if fanarts:
                thumbnails.extend(fanarts)
            if thumbnails:
                result[u'thumb'] = thumbnails
        data = self.user_prefs(details, result)
        return data

    def user_prefs(self, details, result):
        # user preferences
        lang = u'biography' + self.lang
        if self.bio == u'theaudiodb' and u'theaudiodb' in details:
            if lang in details[u'theaudiodb']:
                result[u'biography'] = details[u'theaudiodb'][lang]
            elif u'biographyEN' in details[u'theaudiodb']:
                result[u'biography'] = details[u'theaudiodb'][u'biographyEN']
        elif (self.bio in details) and (u'biography' in details[self.bio]):
            result[u'biography'] = details[self.bio][u'biography']
        if (self.discog in details) and (u'albums' in details[self.discog]):
            result[u'albums'] = details[self.discog][u'albums']
        if (self.genre in details) and (u'genre' in details[self.genre]):
            result[u'genre'] = details[self.genre][u'genre']
        if (self.style in details) and (u'styles' in details[self.style]):
            result[u'styles'] = details[self.style][u'styles']
        if (self.mood in details) and (u'moods' in details[self.mood]):
            result[u'moods'] = details[self.mood][u'moods']
        return result

    def return_search(self, data):
        items = []
        for item in data:
            listitem = xbmcgui.ListItem(item[u'artist'], offscreen=True)
            listitem.setArt({u'thumb': item[u'thumb']})
            listitem.setProperty(u'artist.genre', item[u'genre'])
            listitem.setProperty(u'artist.born', item[u'born'])
            listitem.setProperty(u'relevance', item[u'relevance'])
            if u'type' in item:
                listitem.setProperty(u'artist.type', item[u'type'])
            if u'gender' in item:
                listitem.setProperty(u'artist.gender', item[u'gender'])
            if u'disambiguation' in item:
                listitem.setProperty(u'artist.disambiguation', item[u'disambiguation'])
            url = {u'artist':item[u'artist']}
            if u'mbartistid' in item:
                url[u'mbartistid'] = item[u'mbartistid']
            if u'dcid' in item:
                url[u'dcid'] = item[u'dcid']
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
        if not u'artist' in item:
            return
        listitem = xbmcgui.ListItem(item[u'artist'], offscreen=True)
        if u'mbartistid' in item:
            listitem.setProperty(u'artist.musicbrainzid', item[u'mbartistid'])
        if u'genre' in item:
            listitem.setProperty(u'artist.genre', item[u'genre'])
        if u'biography' in item:
            listitem.setProperty(u'artist.biography', item[u'biography'])
        if u'gender' in item:
            listitem.setProperty(u'artist.gender', item[u'gender'])
        if u'styles' in item:
            listitem.setProperty(u'artist.styles', item[u'styles'])
        if u'moods' in item:
            listitem.setProperty(u'artist.moods', item[u'moods'])
        if u'instruments' in item:
            listitem.setProperty(u'artist.instruments', item[u'instruments'])
        if u'disambiguation' in item:
            listitem.setProperty(u'artist.disambiguation', item[u'disambiguation'])
        if u'type' in item:
            listitem.setProperty(u'artist.type', item[u'type'])
        if u'sortname' in item:
            listitem.setProperty(u'artist.sortname', item[u'sortname'])
        if u'active' in item:
            listitem.setProperty(u'artist.years_active', item[u'active'])
        if u'born' in item:
            listitem.setProperty(u'artist.born', item[u'born'])
        if u'formed' in item:
            listitem.setProperty(u'artist.formed', item[u'formed'])
        if u'died' in item:
            listitem.setProperty(u'artist.died', item[u'died'])
        if u'disbanded' in item:
            listitem.setProperty(u'artist.disbanded', item[u'disbanded'])
        if u'thumb' in item:
            listitem.setProperty(u'artist.thumbs', unicode(len(item[u'thumb'])))
            for count, thumb in enumerate(item[u'thumb']):
                listitem.setProperty(u'artist.thumb%i.url' % (count + 1), thumb[u'image'])
                listitem.setProperty(u'artist.thumb%i.preview' % (count + 1), thumb[u'preview'])
                listitem.setProperty(u'artist.thumb%i.aspect' % (count + 1), thumb[u'aspect'])
        if u'albums' in item:
            listitem.setProperty(u'artist.albums', unicode(len(item[u'albums'])))
            for count, album in enumerate(item[u'albums']):
                listitem.setProperty(u'artist.album%i.title' % (count + 1), album[u'title'])
                listitem.setProperty(u'artist.album%i.year' % (count + 1), album[u'year'])
                if u'musicbrainzreleasegroupid' in album:
                    listitem.setProperty(u'artist.album%i.musicbrainzreleasegroupid' % (count + 1), album[u'musicbrainzreleasegroupid'])
        xbmcplugin.setResolvedUrl(handle=int(sys.argv[1]), succeeded=True, listitem=listitem)
