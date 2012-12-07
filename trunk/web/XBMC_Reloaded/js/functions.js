function charCodetoHuman(key){
	var special_key_names = {
		58: 59,     // : -> ;
        43: 61,     // = -> +
        60: 44,     // < -> ,
        95: 45,     // _ -> -
        62: 46,     // > -> .
        191: 47,     // ? -> /
        96: 192,    // ` -> ~
        124: 92,    // | -> \
        64: 50,     // @ -> 2
        94: 54,     // ^ -> 6
        42: 56,     // * -> 8
        40: 57,     // ( -> 9
        41: 58,     // ) -> 0
        123: 91,    // { -> [
        125: 93,     // } -> ]
		186: 59, // ;: in IE
		187: 61, // =+ in IE
		188: 44, // ,<
		109: 95, // -_ in Mozilla
		107: 61, // =+ in Mozilla
		189: 95, // -_ in IE
		190: 62, // .>
		191: 47, // /?
		192: 126, // `~
		219: 91, // {[
		220: 92, // \|
		221: 93,		
		32: 'SPACE',
		13: 'ENTER',
		9: 'TAB',
		8: 'BACKSPACE',
		16: 'SHIFT',
		17: 'CTRL',
		18: 'ALT',
		20: 'CAPS_LOCK',
		144: 'NUM_LOCK',
		145: 'SCROLL_LOCK',
		37: 'LEFT',
		38: 'UP',
		39: 'RIGHT',
		40: 'DOWN',
		33: 'PAGE_UP',
		34: 'PAGE_DOWN',
		36: 'HOME',
		35: 'END',
		45: 'INSERT',
		46: 'DELETE',
		27: 'ESCAPE',
		19: 'PAUSE',
		222: "'",
		44: "PRINT_SCREEN",
		95: "SlEEP_KEY",
		96: "NUM 0",
		97: "NUM 1",
		98: "NUM 2",
		99: "NUM 3",
		100: "NUM 4", 
		101: "NUM 5",
		102: "NUM 6",
		103: "NUM 7",
		104: "NUM 8",
		105: "NUM 9",
		106: "Multiply key",
		107: "Add key",
		108: "Separator key",
		109: "Subtract key",
		110: "Decimal key",
		111: "Divide key",
		
		112: "F1",
		113: "F2",
		114: "F3",
		115: "F4",
		116: "F5",
		117: "F6",
		118: "F7",
		119: "F8",
		120: "F9",
		121: "F10",
		122: "F11",
		123: "F12",
		124: "F13",
		125: "F14",
		126: "F15",
		127: "F16",
		128: "F17",
		129: "F18",
		130: "F19",
		131: "F20",
		132: "F21",
		133: "F22",
		134: "F23",
		135: "F24",
		189: "-",
		
		173: "Mute Key",
		174: "Volume Down Key",
		175: "Volume Up Key",
		176: "Next Track Key",
		177: "Previous Track Key",
		178: "Stop Media Key",
		179:  "Play/Pause Media Key"
		
		
	};
		if(special_key_names[key]){
				if(isNaN(special_key_names[key])){
					key = special_key_names[key]
				}
				else
				{
					key = String.fromCharCode(special_key_names[key]);
				}
			}
			else
		{
				key = String.fromCharCode(key);
		}
		return key;
}

function SaveKey(thefunction,keycode){
	eval("KeyboardShortcut."+thefunction+" = \""+keycode+"\"");
	var saveforcookie = objectToString(KeyboardShortcut)
	$.cookie('ShotCuts', saveforcookie,{ expires: 365 });
}
function KeyboardControlSaveKey(thefunction,keycode){
	eval("KeyboardController."+thefunction+" = \""+keycode+"\"");
	var saveforcookie = objectToString(KeyboardController)
	$.cookie('KeyboardController', saveforcookie,{ expires: 365 });
	UpdateLegend();
}

function objectToString(o){
 
    var parse = function(_o){
        var a = [], t;
		 for(var p in _o){
            if(_o.hasOwnProperty(p)){
                t = _o[p];
                if(t && typeof t == "object"){
                    a[a.length]= p + ":{ " + arguments.callee(t).join(", ") + "}";   
                }
                else {
                    if(typeof t == "string"){
                        a[a.length] = [ p+ ":" + t.toString() + "" ];
                    }
                    else{
                        a[a.length] = [ p+ ":" + t.toString() +""];
                    }
                }
            }
        }
        return a;
        
    }
	return "{" + parse(o).join(", ") + "}"; 
}

function explode (delimiter, string, limit) {
    // Splits a string on string separator and return array of components. If limit is positive only limit number of components is returned. If limit is negative all components except the last abs(limit) are returned.  
    // 
    // version: 1109.2015
    // discuss at: http://phpjs.org/functions/explode
    // +     original by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
    // +     improved by: kenneth
    // +     improved by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
    // +     improved by: d3x
    // +     bugfixed by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
    // *     example 1: explode(' ', 'Kevin van Zonneveld');
    // *     returns 1: {0: 'Kevin', 1: 'van', 2: 'Zonneveld'}
    // *     example 2: explode('=', 'a=bc=d', 2);
    // *     returns 2: ['a', 'bc=d']
    var emptyArray = {
        0: ''
    };
 
    // third argument is not required
    if (arguments.length < 2 || typeof arguments[0] == 'undefined' || typeof arguments[1] == 'undefined') {
        return null;
    }
 
    if (delimiter === '' || delimiter === false || delimiter === null) {
        return false;
    }
 
    if (typeof delimiter == 'function' || typeof delimiter == 'object' || typeof string == 'function' || typeof string == 'object') {
        return emptyArray;
    }
 
    if (delimiter === true) {
        delimiter = '1';
    }
 
    if (!limit) {
        return string.toString().split(delimiter.toString());
    }
    // support for limit argument
    var splitted = string.toString().split(delimiter.toString());
    var partA = splitted.splice(0, limit - 1);
    var partB = splitted.join(delimiter.toString());
    partA.push(partB);
    return partA;
}
function addslashes(str) {
str=str.replace(/\\/g,'\\\\');
str=str.replace(/\'/g,'\\\'');
str=str.replace(/\"/g,'\\"');
str=str.replace(/\0/g,'\\0');
return str;
}
function stripslashes(str) {
str=str.replace(/\\'/g,'\'');
str=str.replace(/\\"/g,'"');
str=str.replace(/\\0/g,'\0');
str=str.replace(/\\\\/g,'\\');
return str;
}
function isset(varname) {
if(typeof( window[ varname ] ) != "undefined") return true;
else return false;
}
function cleanArray(actual){
  var newArray = new Array();
  for(var i = 0; i<actual.length; i++){
      if (actual[i]){
        newArray.push(actual[i]);
    }
  }
  return newArray;
}
function isArray(obj) {
//returns true is it is an array
if (obj.constructor.toString().indexOf("Array") == -1)
return false;
else
return true;
}