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
		user_agent: document.getElementById( "user_agent" ).checked,
		referer: document.getElementById( "referer" ).checked,
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
		if ( typeof options.server == "undefined" ) { options.server = "http://localhost:80/"; }
		if ( typeof options.username == "undefined" ) { options.username = ""; }
		if ( typeof options.password == "undefined" ) { options.password = ""; }
		if ( typeof options.parts == "undefined" ) { options.parts = "1"; }
		if ( typeof options.default_directory == "undefined" ) { options.default_directory = ""; }
		if ( typeof options.user_agent == "undefined" ) { options.user_agent = true; }
		if ( typeof options.referer == "undefined" ) { options.referer = true; }
		if ( typeof options.override == "undefined" ) { options.override = false; }
		if ( typeof options.show_add_window == "undefined" ) { options.show_add_window = false; }

		document.getElementById( "server" ).value = options.server;
		document.getElementById( "username" ).value = atob( options.username );
		document.getElementById( "password" ).value = atob( options.password );
		document.getElementById( "parts" ).value = options.parts;
		document.getElementById( "default_directory" ).value = atob( options.default_directory );
		document.getElementById( "user_agent" ).checked = options.user_agent;
		document.getElementById( "referer" ).checked = options.referer;
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
