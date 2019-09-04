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
		parts: s_parts,
		default_directory: document.getElementById( "default_directory" ).value,
		user_agent: document.getElementById( "user_agent" ).checked,
		referer: document.getElementById( "referer" ).checked,
		override: document.getElementById( "override" ).checked,
		show_add_window: document.getElementById( "show_add_window" ).checked
	}, function()
	{
		chrome.runtime.sendMessage( { type: "refresh_options" } );
	} );
}

function DisableCheckbox( event )
{
	document.getElementById( "show_add_window" ).disabled = !event.target.checked;
}

function RestoreOptions()
{
	chrome.storage.local.get( null, function( options )
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
	document.querySelectorAll( "[data-i18n]" ).forEach( el =>
	{
		if ( el.id == "default_directory" )
		{
			el.placeholder = chrome.i18n.getMessage( el.dataset.i18n );
		}
		else
		{
			el.innerText = chrome.i18n.getMessage( el.dataset.i18n );
		}
	} );

	document.getElementById( "ok" ).addEventListener( "click", function(){ SaveOptions(); window.close(); } );
	document.getElementById( "cancel" ).addEventListener( "click", function(){ window.close(); } );
	document.getElementById( "apply" ).addEventListener( "click", SaveOptions );
	document.getElementById( "override" ).addEventListener( "change", DisableCheckbox );

	RestoreOptions();
} );
