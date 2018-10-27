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
		parts: s_parts,
		default_directory: document.getElementById( "default_directory" ).value,
		override: document.getElementById( "override" ).checked,
		show_add_window: document.getElementById( "show_add_window" ).checked
	} )
	.then( function()
	{
		browser.runtime.sendMessage( { type: "refresh_options" } );
	} );
}

function DisableCheckbox()
{
	document.getElementById( "show_add_window" ).disabled = !document.getElementById( "override" ).checked;
}

function RestoreOptions()
{
	browser.storage.local.get().then( function( options )
	{
		document.getElementById( "server" ).value = options.server || "http://localhost:80/";
		document.getElementById( "username" ).value = ( options.username ? atob( options.username ) : "" );
		document.getElementById( "password" ).value = ( options.password ? atob( options.password ) : "" );
		document.getElementById( "parts" ).value = options.parts || "1";
		document.getElementById( "default_directory" ).value = ( options.default_directory ? atob( options.default_directory ) : "" );
		document.getElementById( "override" ).checked = options.override;
		document.getElementById( "show_add_window" ).checked = options.show_add_window;
		document.getElementById( "show_add_window" ).disabled = !options.override;
	} );
}

document.addEventListener( "DOMContentLoaded", function()
{
	document.getElementById( "save" ).addEventListener( "click", SaveOptions );
	document.getElementById( "override" ).addEventListener( "change", DisableCheckbox );

	RestoreOptions();
} );
