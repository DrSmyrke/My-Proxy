var requestUpd = makeHttpObject();
var request = makeHttpObject();
var iUpd = 0;



function changeParam( form, elemUpdateID = "viewBox", hidden = false, confirmText = "" )
{
	if( confirmText.length > 0 ){
		if( !confirm( confirmText ) ) return false;
	}
	var viewBoxObj = document.getElementById( elemUpdateID );
	
	viewBoxObj.innerHTML = '<span class="valorange">Processing...</span>';
	
	var arrayInput = form.getElementsByTagName("INPUT");
	var arraySelect = form.getElementsByTagName("SELECT");
	var finalData = Array();
	var finalString = "";
	
	for( i = 0; i < arrayInput.length; i++ ){
		if( arrayInput[i].name == "" ) continue;
		if( arrayInput[i].value != "" ) finalData.push( arrayInput[i].name + "=" + arrayInput[i].value );
	}
	for( i = 0; i < arraySelect.length; i++ ){
		if( arraySelect[i].name == "" ) continue;
		if( arraySelect[i].value != "" ) finalData.push( arraySelect[i].name + "=" + arraySelect[i].value );
	}
	finalString = finalData.join( "&" );
	form.action = form.action + "?" + finalString;
	finalString = null;
	
	request.open("GET", form.action, false);
	request.setRequestHeader('Content-type','application/x-www-form-urlencoded');
	request.send(finalString);
	viewBoxObj.innerHTML = request.responseText;
	
	if( hidden ) setTimeout( "document.getElementById( \"" + elemUpdateID + "\" ).innerHTML='';", 3000 );
	
	return false;
}

function closeEdit()
{
	var edBox = document.getElementById("editorBox");
	
	edBox.style.display = "none";
}

function edit( type, value )
{
	var edBox = document.getElementById("editorBox");
	var edCont = document.getElementById("editorContent");
	var url;
	
	switch( type ){
		case "user":
			url = "/get?userData=" + value;
		break;
		default: return;
	}
	
	edBox.style.display = "block";
	
	request.open( 'GET', url, false );
	request.send( null );
	var tmp = request.responseText.split(":>:");
	edCont.innerHTML = tmp[2];
}

requestUpd.onreadystatechange=function(){
	if (requestUpd.readyState==4 && requestUpd.status == 200) {
		var tmp = requestUpd.responseText.split(":>:");
		if( tmp[0] == "content" ){
			if( tmp.length >= 3 ){
				for( i = 1; i < tmp.length; i++ ){
					if( document.getElementById( tmp[i] ) ) document.getElementById( tmp[i] ).innerHTML = tmp[i+1];
				}
			}
			
			update();
		}
	}
}
	
function update()
{
	if( urlArray.length == 0 ) return;
	if( iUpd >= urlArray.length ){
		iUpd = 0;
		setTimeout( "update()", 10000 );
		return;
	}
	var url = urlArray[iUpd++];
		
	requestUpd.open( 'GET', url, true );
	requestUpd.send( null );
}

function makeHttpObject()
{
	try {return new XMLHttpRequest();}

	catch (error) {}
	try {return new ActiveXObject("Msxml2.XMLHTTP");}
	catch (error) {}

	try {return new ActiveXObject("Microsoft.XMLHTTP");}
	catch (error) {}
	throw new Error("Could not create HTTP request object.");
}
