$(document).ready(function() {
	/* Load & Assign Keyboard shortcuts */
	if($.cookie('ShotCuts')){
		$.cookie('ShotCuts', $.cookie('ShotCuts'),{ expires: 365 });
		shortcuts = $.cookie('ShotCuts').replace('{','').replace('}','');
		var properties = shortcuts.split(', ');
		$.each(properties, function(func,entry){
			 var tup = entry.split(':')
			 SaveKey(tup[0],tup[1])
			 var character = charCodetoHuman(tup[1]);
			 $('#s-'+tup[0]).val(character);
			 $('#kc-s-'+tup[0]).html(tup[1]);
		})
	}	
	if($.cookie('KeyboardController')){
		$.cookie('KeyboardController', $.cookie('KeyboardController'),{ expires: 365 });
		shortcuts = $.cookie('KeyboardController').replace('{','').replace('}','');
		var properties = shortcuts.split(', ');
		$.each(properties, function(func,entry){
			 var tup = entry.split(':')
			 KeyboardControlSaveKey(tup[0],tup[1])
			 var character = charCodetoHuman(tup[1]);
			 $('#c-'+tup[0]).val(character);
			 $('#kc-c-'+tup[0]).html(tup[1]);
		})
	}
	
	windowHeight = $(window).height();
	windowWidth = $(window).width();
	if(windowWidth < 300){
		leftOffset = 1;
	}
	else
	{
		leftOffset = 50;
	}
	dialogWidth = windowWidth*0.45;
	secondDialogPosition = dialogWidth*1.13;
	bHeight = windowHeight*0.19; // Used for the height of Now playing and controls
	cHeight = windowHeight*0.67; // Used for the height of the explorer and playlist
	if(bHeight > 120){
		bHeight = 120
	}
	cTopOffset = bHeight*1.55;
	
	$( "#nowplaying" ).dialog({
								position: [leftOffset, 50], 
								width: dialogWidth,
								height: bHeight,
								draggable: false
	});
	$( "#controls" ).dialog({
								position:[secondDialogPosition, 50], 
								width: dialogWidth,
								height: bHeight,
								draggable: false
	});
  
	
	
	$( "#explorer" ).dialog({
							position: [leftOffset, cTopOffset], 
							width: dialogWidth,
							height: cHeight,
							draggable: false
  });
  $( "#playlist" ).dialog({
							position:[secondDialogPosition, cTopOffset],
							width: dialogWidth,
							height: cHeight,
							draggable: false
  
  });
  
  $( "#Music" ).click(function(){
		loadExplorer("Music",'');
		loadPlaylist("Music");
  })  
  $( "#Video" ).click(function(){
		loadExplorer("Video",'');
		loadPlaylist("Video",'');
  })
  
  
  	$("#ProgressBar").slider({value:0,
					stop: function(event, ui) { 
							SeekPercentage(ui.value)
						}
	});
	
	$('.button').button();
	$("#repeattoggle").toggle(
       function () {
        $(this).button( "option", "label", "Repeat One" );
		$(this).addClass("ui-state-focus")
		doCommand("ExecBuiltIn(PlayerControl(RepeatOne))");
      },
      function () {
		$(this).button( "option", "label", "Repeat All" );
		$(this).addClass("ui-state-focus")
		doCommand("ExecBuiltIn(PlayerControl(RepeatAll))");
      },
	  function () {
        $(this).button( "option", "label", "Repeat None" );
		$(this).removeClass("ui-state-focus")
		doCommand("ExecBuiltIn(PlayerControl(RepeatOff))");
      }
    );	
	$("#shuffletoggle").toggle(
      function () {
		$(this).addClass("ui-state-focus")
		$(this).button( "option", "label", "Shuffle On" );
		doCommand("ExecBuiltIn(PlayerControl(RandomOn))");
		loadPlaylist($(document.body).data("playlist"))
		
      },
      function () {
        $(this).button( "option", "label", "Shuffle Off" );
		$(this).removeClass("ui-state-focus")
		doCommand("ExecBuiltIn(PlayerControl(RandomOff))");
		loadPlaylist($(document.body).data("playlist"))
      }
    );
	
	
	$('.keyconfig').keyup(function(event) {
	  if ( event.which == 13 ) {
		 event.preventDefault();
	   }
		
		if(this.value == '' && event.keyCode == 8){
			key = ''
		}
		else
		{
			key = event.keyCode
		}
		thefunction = this.id.replace('s-','')
		$(this).val(charCodetoHuman(key));
		SaveKey(thefunction,key)
		$('#kc-'+this.id ).html(key);
	  
	});	
	$('.keyconfig').click(function() {       $(this).select(); })
	$('.keyboardcontroller').click(function() {       $(this).select(); })

	$('.keyboardcontroller').keyup(function(event) {
	  if ( event.which == 13 ) {
		 event.preventDefault();
	   }
		
		if(this.value == '' && event.keyCode == 8){
			key = ''
		}
		else
		{
			key = event.keyCode
		}
		thefunction = this.id.replace('c-','')
		$(this).val(charCodetoHuman(key));
		KeyboardControlSaveKey(thefunction,key)
		$('#kc-'+this.id ).html(key);
	  
	});
	$.data(document.body,'Settingactive',0)
	$.data(document.body,'keyboardcontrol',0)
	
	$('#SettingsModal').on('hidden', function () {
		$.data(document.body, 'Settingactive', 0)
	});	
	
	
	$('#KeyboardControl').on('hidden', function () {
		$.data(document.body, 'keyboardcontrol', 0)
	});
	
	$(document.body).keydown(function(event) {
		var SearchFocusCheck = $(".search-query").is(":focus");
		if($.data(document.body,'Settingactive') == 0 && SearchFocusCheck == false && $.data(document.body,'keyboardcontrol') == 0){
			$.each(KeyboardShortcut,function(theaction,key){
				if(event.keyCode == key){
					eval(theaction+"();");
					
				}
			})
		}
		if($.data(document.body,'keyboardcontrol') == 1){
			
			if($('#bsettings').hasClass("active")){
			
			}
			else
			{
				keyboardControl(event);
				event.preventDefault();
			}
		}
	})
	
	$('.search-query').keypress(function(event){
		if (event.keyCode == 13) {
			event.preventDefault();
			Search(this.value);
			$(this).val('');
			this.blur();
		}
	})
	
	GetVolume()
	GetCurrentlyPlaying()
	//Set output out search correctly:
	doCommand("setresponseformat(openRecordSet;<recordset>;closeRecordSet;</recordset>;openRecord;<record>;closeRecord;</record>;openField;<field>;closeField;</field>)")
});

$(window).resize(function() {
	windowHeight = $(window).height();
	windowWidth = $(window).width();
	if(windowWidth < 300){
		leftOffset = 1;
	}
	else
	{
		leftOffset = 50;
	}
	dialogWidth = windowWidth*0.45;
	secondDialogPosition = dialogWidth*1.13;
	bHeight = windowHeight*0.19; // Used for the height of Now playing and controls
	cHeight = windowHeight*0.67; // Used for the height of the explorer and playlist
	if(bHeight > 120){
		bHeight = 120
	}
	cTopOffset = bHeight*1.55;

	$( "#nowplaying" ).dialog("option", "position", [leftOffset, 50])
	$( "#nowplaying" ).dialog("option", "width", dialogWidth)
	$( "#nowplaying" ).dialog("option", "height", bHeight)

	$( "#controls" ).dialog("option", "position",[secondDialogPosition, 50])
	$( "#controls" ).dialog("option", "width",dialogWidth)
	$( "#controls" ).dialog("option", "height",bHeight)
	
	$( "#explorer" ).dialog("option", "position",[leftOffset, cTopOffset])
	$( "#explorer" ).dialog("option", "width",dialogWidth)
	$( "#explorer" ).dialog("option", "height",cHeight)
						
	$( "#playlist" ).dialog("option", "position",[secondDialogPosition, cTopOffset])
	$( "#playlist" ).dialog("option", "width",dialogWidth)
	$( "#playlist" ).dialog("option", "height",cHeight)
});

function ShowSettings(){
	$('#SettingsModal').modal();
	$.data(document.body, 'Settingactive', 1)
}