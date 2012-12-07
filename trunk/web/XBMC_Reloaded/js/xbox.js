XBOXIP = window.location.host // Enter your XBOX ip. When you use a port enter it like 127.0.0.1:8080


//DO NOT EDIT BELOW
function doCommand(command){
		var result = null;
		//command = encodeURIComponent(command);
	//	command = escape(command);

		var scriptUrl = "http://"+ XBOXIP +"/xbmcCmds/xbmcHttp?command="+command;
			 $.ajax({
				url: scriptUrl,
				type: 'get',
				dataType: 'html',
				async: false,
				timeout: 3000,
				success: function(data) {
					result = data;
				} 
		});
		result = result.toString();
		response = result.substr(6,result.length);
		response = response.substr(0,response.length-8)
		response = response.replace(/(\r\n|\n|\r)/gm,"");
		response = response.split('<li>');

		if(response.length == 0){
			response = result
		}else{
			response = response.splice(1, response.length);
		}
		return response
}
function GetCurrentlyPlaying(){
	curplaying = Array();
	NowPlaying = new Object();
	currentplaying = doCommand("GetCurrentlyPlaying");
	$.each(currentplaying,function(key,attribute){
		attribute = explode(":",attribute,2);
		//$.data(document.body, attribute[0], attribute[1]);
		Propertyname = attribute[0].replace(' ','')
		if(Propertyname != "Plot"){
			eval("NowPlaying."+Propertyname+" = \""+attribute[1]+"\"");
		}
	})
	
	var filename = NowPlaying.Filename
	
	if(filename != "[Nothing Playing]"){
		var album = (NowPlaying.Album) ? ' ('+NowPlaying.Album+')' : '';
		var artist = (NowPlaying.Artist) ? NowPlaying.Artist : '';
		var title = (NowPlaying.Title) ? NowPlaying.Title : filename.replace(/^.*[\\\/]/, '');
		var samplerate = NowPlaying.Samplerate
		var bitrate = NowPlaying.Bitrate
		var genre = NowPlaying.Genre
		var percentage = NowPlaying.Percentage
		var playstatus = NowPlaying.PlayStatus
		var duration = NowPlaying.Duration
		var playtime = NowPlaying.Time
		var type = NowPlaying.Type
		
		if(playstatus == "Playing"){
				$('#Playimg').attr('src','images/pause.png');
		}
		else
		{
			$('#Playimg').attr('src','images/play.png');
		}
		if(type == "Audio"){
			
			nowplaying = (artist) ? artist + " - " + title + album: title+album;
			var playlistindex = NowPlaying.SongNo
		}
		else if(type == "Video"){
			var filename = filename.replace(/^.*[\\\/]/, '');
			nowplaying = filename;
			var playlistindex = NowPlaying.VideoNo
		}
		$('li[id|="playliste"]').removeClass("ui-state-focus")
		$('#playliste-'+ playlistindex).addClass("ui-state-focus")
		
		$( "#ProgressBar" ).slider( "option", "value", percentage );
		
	}
	else
	{
		nowplaying = "Nothing playing";
		duration = '0:00';
		playtime = '0:00';
		$( "#ProgressBar" ).slider( "option", "value", 0 );
	}
	$("#curplaying").html(nowplaying);
	$("#Time").html(playtime+ " / "+ duration);
}

function playFile(file,playlist){
	file = stripslashes(file)
	//file = escape(file)
	file = encodeURIComponent(file);
	//file = file.replace(/\&/g,'\%26');
	
	doCommand("PlayFile("+file+";"+playlist+")");
}

function PlayPrev(){
	doCommand("PlayListPrev");
}

function SeekPercentage(percent){
	doCommand("SeekPercentage("+percent+")");
}

function SeekBackward(){
	doCommand("SeekPercentageRelative(-5)")
}
function Stop(){
	doCommand("Stop()");
}
function Pause(){
	doCommand("Pause()");
	$.data(document.body,"paused",1);
}
function PlayPause(){
	playstatus = NowPlaying.PlayStatus;
	if(playstatus == "Playing"){
		Pause();
	}
	else
	{
		if($.data(document.body,"paused") == 1){
			Pause();	
			$.data(document.body,"paused",0);
		}
		else
		{
		Play();
		}
	}
}

function Play(){
	doCommand("play")
}
function SeekForward(){
	doCommand("SeekPercentageRelative(5)");
}
function Next(){
	doCommand("PlayListNext");
}
function Voldown(){
	volume = $.data(document.body,"Volume")*1;
	volume = volume-5;
	$.data(document.body,"Volume",volume);
	doCommand("SetVolume("+volume+")");
	$("#volume").text(volume);
	
}
function Volup(){
	volume = $.data(document.body,"Volume")*1;
	volume = volume+5;
	$.data(document.body,"Volume",volume);
	doCommand("SetVolume("+volume+")");
	$("#volume").text(volume);
}
function Mute(){
	doCommand("Mute()");
	volume = doCommand("GetVolume").toString();
	$("#volume").text(volume);
	$.data(document.body,"Volume",volume);
}
function GetVolume(){
	volume = doCommand("GetVolume").toString();
	$("#volume").text(volume);
	$.data(document.body, "Volume", volume);
}
function goToByScroll(id){
console.debug( $("#"+id).position().top)
    $('#fileexplorer').animate({
        scrollTop: $("#"+id).position().top},
        'slow');
}

function jqscroll(id){
	$('#'+id).prepend('<a class="highlighted" name="hightlighted" />');
	window.location.hash = '#highlighted';
}

function loadExplorer(type, path){
	path = stripslashes(path)
	navix = '<a href="#" onclick="javascript:$(\'#'+type+'\').click()">'+type+'</a>'
	if (path.substr(0, 3) == "smb") {
		console.debug('SMB Share')
		navi = path.split('/');
		//navi = navi.reverse()
		navi = cleanArray(navi);
		server = navi[0]+'//'+navi[1]+'/'
		delete navi[0]
		delete navi[1]
		navi = cleanArray(navi);
		
		console.debug(navi);
		
		navipath = server+''
		
		$.each(navi,function(key,dir){
			navipath = navipath+dir+'/'
			navix += ' >> <a href="#" onclick="loadExplorer(\''+type+'\',\''+navipath+'\')">'+ dir +'</a>'
		})	
	}
	else{
		console.debug(path);
		navi = path.split('\\');
		server = navi[0]+'\\'
		delete navi[0]
		navi = cleanArray(navi);
		console.debug(navi);
		
		navipath = server+''
		
		$.each(navi,function(key,dir){
			navipath = navipath+dir+'\\'
			navix += ' >> <a href="#" onclick="loadExplorer(\''+type+'\',\''+addslashes(navipath)+'\')">'+ dir +'</a>'
		})
	}
	$('#navi').html(navix);
	locations =  doCommand("GetMediaLocation("+type+";"+path+")");
	var list = '';
	
	//FILE EXTENSIONS Array
		//FILE EXTENSIONS Array
	if(type == "Music"){
		extensionlist =  new Array("strm","divx","mp3","aac","flac","mpa","ogg","pls","ra","ram","wav","wma","aa3","acm","m3u","m4a","m4b","midi","mid");
		thepl = 0;
	}
	else if(type == "Video"){
		extensionlist =  new Array("strm","divx","avi","3gp2","3gp","asx","strm","amx","avi","f4v","flv","gvp","wma","m4e","mp4","m4u","m4v","mkv","mov","mpeg","mpg","mts","ogm","wvx","yuv")
		thepl = 1;
	}
	
	
	$.each(locations,function(key,location){
			location = explode(";",location,3);
			
			fileexension = location[0].split('.').pop();
			
			if( $.inArray(fileexension, extensionlist) !== -1 ) {
				list += '<img src="images/'+ type +'.png"> <a href="#" onclick="addToPlaylist(\''+ addslashes(location[1]) +'\',\''+type+'\'); return false;"><img src="images/add.png" border="0"></a> <a href="#" onclick="playFile(\''+addslashes(location[1])+'\',\''+thepl+'\'); return false;">'+location[0]+'</a><br>'; 
			}
			else
			{
				idst = location[0].replace(/ /g,'-');
				list += '<span id='+ idst +'><img src="images/Folder.png"><a href="#" onclick="addToPlaylist(\''+ addslashes(location[1]) +'\',\''+type+'\')""><img src="images/add.png" border="0"></a><a href="#" onclick="loadExplorer(\''+type+'\',\''+addslashes(location[1])+'\'); return false;">'+location[0]+'</a></span><br>'; 
			}
	})
	$('#fileexplorer').html(list);
	
}



function addToPlaylist(dir,type){

	if(type == "Music"){
		playlist = 0;
	}
	else
	{
		playlist = 1;
	}
	dir = stripslashes(dir)
	dir = encodeURIComponent(dir);
	result = doCommand("AddToPlayList("+ dir +";"+ playlist +";["+ type +"];1)")
	loadPlaylist(type)
}
function addSongToPlaylistById(id){
	doCommand("AddToPlayListFromDB(songs;idSong="+id+")")
	loadPlaylist(0)
}
function ClearPlayList(playlist){
	doCommand("ClearPlayList("+playlist+")")
	loadPlaylist(playlist)
}

function RemoveFromPlaylist(item,playlist){
	doCommand("RemoveFromPlaylist("+ item +";"+ playlist +")")
	loadPlaylist(playlist)
}
function SetPlaylistSong(song){
	doCommand("SetPlaylistSong("+ song +")")
}
function loadPlaylist(playlist){
	$( "#rmpl" ).remove();
	$(document.body).data("playlist",playlist);
	if(playlist=="Music" || playlist == 0){
		nplaylist = 0;
		$( "#playlist" ).dialog( "option", "title", 'Music Playlist' );
	}
	else
	{
		nplaylist = 1;
		$( "#playlist" ).dialog( "option", "title", 'Video Playlist' );
	}
	$( "#playlist" ).prepend('<a href="#" onclick="ClearPlayList('+ nplaylist + '); return false;" id="rmpl">Clear playlist</a>');
	playlist = doCommand("SetCurrentPlaylist("+nplaylist+")")
	playlist = doCommand("GetPlaylistContents("+nplaylist+")")
	
	var playlisthtml = ''
	var i = 0;
	$.each(playlist,function(key,entry){
	
		controls = '<span><a href="#" onClick="SetPlaylistSong(\''+i+'\'); return false;"><img src="images/play.png" height="16" width="16"></a> <a href="#" onClick="RemoveFromPlaylist(\''+addslashes(entry)+'\',\''+nplaylist+'\'); return false;"><img src="images/remove.png" height="16" width="16" border="0"></a></span>'
		playlisthtml += '<li id="playliste-'+ i + '">'+ controls +' '+ entry.replace(/^.*[\\\/]/, '') +'</li>';
		i = i+1;
	})
	
	
	
	$("#playlistContent").html(playlisthtml);
	//RemoveFromPlaylist(item,[playlist])
	//Make hovers in the playlist
	$('li[id*="playliste-"]').hover(
		  function () {
				$(this).addClass( "ui-state-active" )
				var file = $(this).attr('file')
				
				
				
		  }, 
		  function () {
				$(this).removeClass( "ui-state-active" )
		  }
	);
}
function GetSystemInfo(){
	var info = doCommand("GetSystemInfo(112;113;120;121)")
	dockhtml = 'CPU Temp: '+info[0] +' | GPU Temp:'+ info[1] +' | Build version: '+ info[2] +'@'+info[3];
//	$("#SystemInfo").html(dockhtml);
}

function Restart(){
	doCommand("Restart()");
}

function Shutdown(){
	doCommand("Shutdown()");
}
function ToDash(){
	doCommand("Exit()");
}
function Query(database,sql){
	var response = null;
	var scriptUrl = "http://"+ XBOXIP +"/xbmcCmds/xbmcHttp?command=Query"+database+"Database("+sql+")"
			 $.ajax({
				url: scriptUrl,
				type: 'get',
				dataType: 'html',
				async: false,
				success: function(data) {
					response = data;
				} 
		});
	response = response.toString();
	result = response.substr(6,response.length);
	result = result.substr(0,result.length-8)
	result = result.replace(/<\/record>/gm,'');
	result = result.replace('<recordset>','');
	result = result.replace(/(\r\n|\n|\r)/gm,"");
	result = result.replace('</recordset>','');
	result = result.split('<record>')
	result = cleanArray(result);
	resultA = new Array()
	$.each(result,function(index,row){
			row = row.replace(/<\/field>/gm,'');
			row = row.split('<field>');
			row = cleanArray(row);
			resultA.push(row);
	})
	
	return resultA;
}

function Search(searchq){
	console.debug('Search for: '+searchq);
	var musicQuery = "select strArtist,strTitle,strPath,strFileName,iTimesPlayed,idSong from song left join path on song.idPath = path.idPath left join artist on song.idArtist=artist.idArtist where strTitle like '%"+ searchq +"%' OR strArtist like '%"+ searchq +"%' OR strFileName like '%"+ searchq +"%' ORDER BY iTimesPlayed LIMIT 500";
	musicQuery = musicQuery.replace(' ','%20');
	Musicresult = Query("Music",musicQuery);
	
	list = 'Search results for: '+ searchq +' ('+ Musicresult.length+' results)<hr>';

	$.each(Musicresult,function(key,row){
		console.debug(row)
		list += '<img src="images/Music.png"> <a href="#" onclick="addSongToPlaylistById(\''+ row[5] +'\'); return false;"><img src="images/add.png" border="0"></a> <a href="#" onclick="playFile(\''+row[2]+row[3]+'\',\'1\'); return false;">'+row[0]+' - ' +row[1] +'</a><br>'; 
	})
	$('#fileexplorer').html(list);
}

function ToggleRepeat(){
	$('#repeattoggle').click();
}function ToggleShuffle(){
	$('#shuffletoggle').click();
}

function ProfileBackup(){
	$('#profilebackup').html("Backup started");
	response = doCommand("FileCopy(Q:\\System\\Profiles.xml;Q:\\web\\reloaded\\backups\\profiles.xml)");
	if(response[0]=="OK"){
		$('#profilebackup').html('Backup finished, <a href="backups/profiles.xml">Right click here, and save as</a>')
	}
}

function Backup(type){
	$('#'+type+'Backup').html("Backup started");
	response = doCommand("ExecBuiltIn(exportlibrary("+type+",FALSE,Q:\\web\\reloaded\\backups\\"+type+"db))")
	if(response[0]=="OK"){
		$('#'+type+'Backup').html('Backup finished, <a href="backups/'+type+'/">Click here to view</a>')
	}
}
function KeyboardControlModal(){
	$.data(document.body,"keyboardcontrol",1)
	$('#KeyboardControl').modal('show');
}
function SendKey(key){
	key = key.toString();
	doCommand("SendKey("+key+")");
	
}