"""
   Module retrieving repositories info from XBMC Wiki
   by Temhil
"""
__all__ = [
    # public names
    "ListItemFromWiki"
    ]

from BeautifulSoup import SoupStrainer, BeautifulSoup
from traceback import print_exc
import re
import urllib

# Custom modules
if ( __name__ != "__main__" ):
    from Item import TYPE_ADDON_REPO

class ListItemFromWiki:
    currentParseIdx = 1
    addons = []
    def __init__( self, pageUrl ):
        try:
            if ( pageUrl ):
                # Get HTML page...
                htmlSource = urllib.urlopen( pageUrl ).read()

                # Parse response...
                beautifulSoup = BeautifulSoup( htmlSource )
                self.itemRepoList = beautifulSoup.findAll("tr")
        except:
            status = 'ERROR'
            print_exc()


    def _parseRepoElement(self, repoElt, repoInfo):
        status = 'OK'
        try:
            tdList = repoElt.findAll( ["th", "td"] )
            if tdList:
                try:
                    repoInfo[ "name" ]        = tdList[0].a.string.strip()
                    repoInfo[ "description" ] = tdList[1].string.strip()
                    repoInfo[ "author" ]      = tdList[2].string.strip()

                except:
                    repoInfo[ "name" ]        = None
                    repoInfo[ "description" ] = None
                    repoInfo[ "author" ]      = None
                
                try:
                    repoInfo[ "repo_url" ] = tdList[3].a["href"]
                except:
                    repoInfo[ "repo_url" ] = None

                repoInfo[ "version" ]     = None
                repoInfo[ "type" ]        = TYPE_ADDON_REPO

                try:
                    repoInfo[ "ImageUrl" ] = tdList[4].a[ "href" ]
                except:
                    repoInfo[ "ImageUrl" ] = None
        except:
            xbmc.log("_parseRepoElement - error parsing html - impossible to retrieve Repos info", xbmc.LOGNOTICE)
            print_exc()
            result = "ERROR"

        return status


    def getNextItem(self):
        """
        return the next Repository in the list or return 'None' when no repository is left
        """
        result = None
        if len(self.itemRepoList) > 0 and self.currentParseIdx < len(self.itemRepoList):
            itemInfo = {}
            status = self._parseRepoElement( self.itemRepoList[self.currentParseIdx], itemInfo )
            self.currentParseIdx = self.currentParseIdx + 1
            if status == 'OK':
                result = itemInfo
        return result



if ( __name__ == "__main__" ):
    TYPE_ADDON_REPO = "repository"
    print "Wiki parser test"

    repoListUrl = "http://wiki.xbmc.org/index.php?title=Unofficial_Add-on_Repositories"
    print repoListUrl

    listRepoWiki = ListItemFromWiki(repoListUrl)
    keepParsing = True
    while (keepParsing):
        item = listRepoWiki.getNextItem()
        print item
        if item:
            print "Item OK"
        else:
            keepParsing = False


