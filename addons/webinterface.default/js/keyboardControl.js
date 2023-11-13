// Analogue - don't change order
var KEY_BUTTON_A              =          256
var KEY_BUTTON_B              =          257
var KEY_BUTTON_X              =          258
var KEY_BUTTON_Y              =          259
var KEY_BUTTON_BLACK          =          260
var KEY_BUTTON_WHITE          =          261
var KEY_BUTTON_LEFT_TRIGGER    =         262
var KEY_BUTTON_RIGHT_TRIGGER   =         263

var KEY_BUTTON_LEFT_THUMB_STICK  =       264
var KEY_BUTTON_RIGHT_THUMB_STICK   =     265

var KEY_BUTTON_RIGHT_THUMB_STICK_UP  =   266 // right thumb stick directions
var KEY_BUTTON_RIGHT_THUMB_STICK_DOWN  = 267 // for defining different actions per direction
var KEY_BUTTON_RIGHT_THUMB_STICK_LEFT  = 268
var KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT = 269

// Digital - don't change order
var KEY_BUTTON_DPAD_UP         =         270
var KEY_BUTTON_DPAD_DOWN       =         271
var KEY_BUTTON_DPAD_LEFT      =          272
var KEY_BUTTON_DPAD_RIGHT     =          273

var KEY_BUTTON_START          =          274
var KEY_BUTTON_BACK           =          275

var ACTION_VOLUME_UP    =       88
var ACTION_VOLUME_DOWN     =     89
var ACTION_MUTE         =       91

function keyboardControl(event){
	$.each(KeyboardController,function(theaction,key){
				if(event.keyCode == key){
					if(theaction.substr(0,3) == "KEY"){
						$('#keySendlog').val('Key: ' + theaction + ' RAWID: ' +eval(theaction)+'\n'+$('#keySendlog').val());
						SendKey(eval(theaction))
					}
					if(theaction.substr(0,6) == "ACTION"){
						$('#keySendlog').val('ACTION: ' + theaction + ' RAWID: ' +eval(theaction)+'\n'+$('#keySendlog').val());		
						doCommand("Action("+eval(theaction)+")");
					}
				}
				
			})
}

function UpdateLegend(){
	var legend = "";
	$.each(KeyboardController,function(theaction,key){
			key = charCodetoHuman(key)
			action = theaction.replace('KEY','').replace(/_/gm,' ').replace('BUTTON','').replace('ACTION','').toLowerCase().toString().capitalize();
			legend = legend+"<b>"+action +":</b> "+key+"<br />";
	})
	$('#keyboardcontroller-Legend').html(legend);
}
String.prototype.capitalize = function(){
   return this.replace( /(^|\s)([a-z])/g , function(m,p1,p2){ return p1+p2.toUpperCase(); } );
  };