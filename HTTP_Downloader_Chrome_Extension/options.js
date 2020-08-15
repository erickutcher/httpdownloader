function SaveOptions()
{
	chrome.storage.local.set(
	{
		server: document.getElementById( "server" ).value,
		username: btoa( document.getElementById( "username" ).value ),
		password: btoa( document.getElementById( "password" ).value ),
		user_agent: document.getElementById( "user_agent" ).checked,
		referer: document.getElementById( "referer" ).checked,
		override: document.getElementById( "override" ).checked,
		override_prompts: document.getElementById( "override_prompts" ).checked,
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
		if ( typeof options.user_agent == "undefined" ) { options.user_agent = true; }
		if ( typeof options.referer == "undefined" ) { options.referer = true; }
		if ( typeof options.override == "undefined" ) { options.override = false; }
		if ( typeof options.override_prompts == "undefined" ) { options.override_prompts = true; }
		if ( typeof options.show_add_window == "undefined" ) { options.show_add_window = false; }

		document.getElementById( "server" ).value = options.server;
		document.getElementById( "username" ).value = atob( options.username );
		document.getElementById( "password" ).value = atob( options.password );
		document.getElementById( "user_agent" ).checked = options.user_agent;
		document.getElementById( "referer" ).checked = options.referer;
		document.getElementById( "override" ).checked = options.override;
		document.getElementById( "override_prompts" ).checked = options.override_prompts;
		document.getElementById( "show_add_window" ).checked = options.show_add_window;
		document.getElementById( "show_add_window" ).disabled = !options.override;
	} );
}

document.addEventListener( "DOMContentLoaded", function()
{
	document.querySelectorAll( "[data-i18n]" ).forEach( el =>
	{
		el.innerText = chrome.i18n.getMessage( el.dataset.i18n );
	} );

	document.getElementById( "ok" ).addEventListener( "click", function(){ SaveOptions(); window.close(); } );
	document.getElementById( "cancel" ).addEventListener( "click", function(){ window.close(); } );
	document.getElementById( "apply" ).addEventListener( "click", SaveOptions );
	document.getElementById( "override" ).addEventListener( "change", DisableCheckbox );

	RestoreOptions();
} );
