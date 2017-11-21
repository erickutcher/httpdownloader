function SaveOptions()
{
	var s_parts = document.getElementById( "parts" ).value;
	var i_parts = parseInt( s_parts );
	if ( isNaN( i_parts ) || i_parts < 0 )
	{
		s_parts = "0";	// Default.
	}
	else if ( i_parts > 100 )
	{
		s_parts = "100";	// Max.
	}

	chrome.storage.local.set(
	{
		server: document.getElementById( "server" ).value,
		username: btoa( document.getElementById( "username" ).value ),
		password: btoa( document.getElementById( "password" ).value ),
		parts: s_parts
	} );
}

function RestoreOptions()
{
	chrome.storage.local.get( null, function( options )
	{
		document.getElementById( "server" ).value = options.server || "http://localhost:80/";
		document.getElementById( "username" ).value = ( options.username ? atob( options.username ) : "" );
		document.getElementById( "password" ).value = ( options.password ? atob( options.password ) : "" );
		document.getElementById( "parts" ).value = options.parts || "1";
	} );
}

document.addEventListener( "DOMContentLoaded", function()
{
	document.getElementById( "ok" ).addEventListener( "click", function(){ SaveOptions(); window.close(); } );
	document.getElementById( "cancel" ).addEventListener( "click", function(){ window.close(); } );
	document.getElementById( "apply" ).addEventListener( "click", SaveOptions );

	RestoreOptions();
} );
