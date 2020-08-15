function UpdateTheme( show )
{
	if ( show )
	{
		browser.theme.getCurrent().then( function ( theme )
		{
			if ( theme.colors )
			{
				var style = document.getElementById( "theme" );
				if ( style == null )
				{
					style = document.createElement( "style" );
					style.id = "theme";
				}
				else if ( style.firstChild != null )
				{
					style.removeChild( style.firstChild );
				}

				var style_text = "";
				
				var body = "";
				if ( theme.colors.popup ) { body += "background-color: " + theme.colors.popup + ";"; }
				if ( theme.colors.popup_text ) { body += "color: " + theme.colors.popup_text + ";"; }
				else if ( theme.colors.toolbar_text ) { body += "color: " + theme.colors.toolbar_text + ";"; }

				var inputs = "";
				if ( theme.colors.sidebar ) { inputs += "background-color: " + theme.colors.sidebar + ";"; }
				if ( theme.colors.sidebar_border ) { inputs += "border-color: " + theme.colors.sidebar_border + ";"; }
				if ( theme.colors.sidebar_text ) { inputs += "color: " + theme.colors.sidebar_text + ";"; }

				var inputs_focus = "";
				if ( theme.colors.toolbar_field_border_focus ) { inputs_focus += "border-color: " + theme.colors.toolbar_field_border_focus + ";"; }

				var fieldset = "";
				if ( theme.colors.toolbar_field_border ) { fieldset += "border-color: " + theme.colors.toolbar_field_border + ";"; }

				var section_separator = "";
				if ( theme.colors.toolbar_field_border ) { section_separator += "border-color: " + theme.colors.toolbar_field_border + ";"; }

				if ( body != "" ) { style_text += "body {" + body + "}"; }
				if ( inputs != "" ) { style_text += "input, textarea, select {" + inputs + "}"; }
				if ( inputs_focus != "" ) { style_text += "input:focus, textarea:focus, select:focus {" + inputs_focus + "}"; }
				if ( fieldset != "" ) { style_text += "fieldset {" + fieldset + "}"; }
				if ( section_separator != "" ) { style_text += ".section-separator {" + section_separator + "}"; }

				if ( style_text != "" )
				{
					style.appendChild( document.createTextNode( style_text ) );
					document.getElementsByTagName( "head" )[ 0 ].appendChild( style );
				}
			}
			else
			{
				var style = document.getElementById( "theme" );
				if ( style != null )
				{
					style.remove();
				}
			}
		} );
	}
	else
	{
		var style = document.getElementById( "theme" );
		if ( style != null )
		{
			style.remove();
		}
	}
}

function SaveOptions()
{
	browser.storage.local.set(
	{
		server: document.getElementById( "server" ).value,
		username: btoa( document.getElementById( "username" ).value ),
		password: btoa( document.getElementById( "password" ).value ),
		user_agent: document.getElementById( "user_agent" ).checked,
		referer: document.getElementById( "referer" ).checked,
		override: document.getElementById( "override" ).checked,
		override_prompts: document.getElementById( "override_prompts" ).checked,
		theme_support: document.getElementById( "theme_support" ).checked,
		show_add_window: document.getElementById( "show_add_window" ).checked
	} )
	.then( function()
	{
		UpdateTheme( document.getElementById( "theme_support" ).checked );

		browser.runtime.sendMessage( { type: "refresh_options" } );
	} );
}

function DisableCheckbox( event )
{
	document.getElementById( "show_add_window" ).disabled = !event.target.checked;
}

function RestoreOptions()
{
	browser.storage.local.get().then( function( options )
	{
		if ( typeof options.server == "undefined" ) { options.server = "http://localhost:80/"; }
		if ( typeof options.username == "undefined" ) { options.username = ""; }
		if ( typeof options.password == "undefined" ) { options.password = ""; }
		if ( typeof options.user_agent == "undefined" ) { options.user_agent = true; }
		if ( typeof options.referer == "undefined" ) { options.referer = true; }
		if ( typeof options.override == "undefined" ) { options.override = false; }
		if ( typeof options.override_prompts == "undefined" ) { options.override_prompts = true; }
		if ( typeof options.theme_support == "undefined" ) { options.theme_support = true; }
		if ( typeof options.show_add_window == "undefined" ) { options.show_add_window = false; }

		document.getElementById( "server" ).value = options.server;
		document.getElementById( "username" ).value = atob( options.username );
		document.getElementById( "password" ).value = atob( options.password );
		document.getElementById( "user_agent" ).checked = options.user_agent;
		document.getElementById( "referer" ).checked = options.referer;
		document.getElementById( "override" ).checked = options.override;
		document.getElementById( "override_prompts" ).checked = options.override_prompts;
		document.getElementById( "theme_support" ).checked = options.theme_support;
		document.getElementById( "show_add_window" ).checked = options.show_add_window;
		document.getElementById( "show_add_window" ).disabled = !options.override;
	} );
}

document.addEventListener( "DOMContentLoaded", function()
{
	browser.storage.local.get().then( function( options )
	{
		UpdateTheme( ( typeof options.theme_support != "undefined" ? options.theme_support : false ) );
	} );

	document.querySelectorAll( "[data-i18n]" ).forEach( el =>
	{
		el.innerText = browser.i18n.getMessage( el.dataset.i18n );
	} );

	document.getElementById( "save" ).addEventListener( "click", SaveOptions );
	document.getElementById( "override" ).addEventListener( "change", DisableCheckbox );

	RestoreOptions();
} );
