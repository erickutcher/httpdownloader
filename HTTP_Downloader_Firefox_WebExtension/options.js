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

	browser.storage.local.set(
	{
		server: document.getElementById( "server" ).value,
		username: btoa( document.getElementById( "username" ).value ),
		password: btoa( document.getElementById( "password" ).value ),
		parts: s_parts
	} );
}

function RestoreOptions()
{
	browser.storage.local.get().then( function( options )
	{
		document.getElementById( "server" ).value = options.server || "http://localhost:80/";
		document.getElementById( "username" ).value = ( options.username ? atob( options.username ) : "" );
		document.getElementById( "password" ).value = ( options.password ? atob( options.password ) : "" );
		document.getElementById( "parts" ).value = options.parts || "1";
	} );
}

document.addEventListener( "DOMContentLoaded", function()
{
	document.getElementById( "save" ).addEventListener( "click", SaveOptions );

	RestoreOptions();
} );
