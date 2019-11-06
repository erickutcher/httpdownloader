var refresh_server_info = false;

var g_initial_height = 0;

function SendDownloadToClient( add_type )
{
	var request = new XMLHttpRequest();
	if ( request )
	{
		var window_id = parseInt( document.getElementById( "window_id" ).value );
		var method = "1";	// GET
		var post_data = "";
		if ( document.getElementById( "method" ).checked )
		{
			method = "2";	// POST
			post_data = document.getElementById( "post_data" ).value;
		}
		var server = document.getElementById( "server" ).value;
		var cookies = document.getElementById( "cookies" ).value;
		var headers = document.getElementById( "headers" ).value;
		if ( headers != "" )
		{
			headers = headers.trim();
			headers = headers.replace( /\r?\n/g, "\r\n" );
			headers = headers.replace( /(\r\n)+/g, "\r\n" );
			headers += "\r\n";
		}
		var server_username = document.getElementById( "server_username" ).value;
		var server_password = document.getElementById( "server_password" ).value;

		var urls = document.getElementById( "urls" ).value;
		if ( urls != "" )
		{
			urls = urls.trim();
			urls = urls.replace( /\r?\n/g, "\r\n" );
			urls = urls.replace( /(\r\n)+/g, "\r\n" );
			urls += "\r\n";
		}

		var username = document.getElementById( "username" ).value;
		var password = document.getElementById( "password" ).value;
		var parts = document.getElementById( "parts" ).value;
		var speed_limit = document.getElementById( "speed_limit" ).value;
		var directory = document.getElementById( "directory" ).value;
		var download_operations = add_type | ( document.getElementById( "simulate_download" ).checked ? 1 : 0 );

		var download = document.getElementById( "download" );
		download.disabled = true;

		var add = document.getElementById( "add" );
		add.disabled = true;

		request.onerror = function( e )
		{
			refresh_server_info = true;

			download.disabled = false;
			add.disabled = false;

			window.alert( browser.i18n.getMessage( "SEND_FAILED" ) );
		};

		request.ontimeout = function( e )
		{
			refresh_server_info = true;

			download.disabled = false;
			add.disabled = false;

			window.alert( browser.i18n.getMessage( "CONNECTION_TIMEOUT" ) );
		};

		request.onload = function( e )
		{
			if ( request.responseText == "DOWNLOADING" )
			{
				refresh_server_info = false;

				browser.windows.remove( window_id );
			}
			else
			{
				refresh_server_info = true;

				download.disabled = false;
				add.disabled = false;

				window.alert( browser.i18n.getMessage( "INVALID_RESPONSE" ) );
			}
		};

		if ( server_username != "" || server_password != "" )
		{
			request.open( "POST", server, true, server_username, server_password );
			request.withCredentials = true;
		}
		else
		{
			request.open( "POST", server, true );
		}
		request.timeout = 30000;	// 30 second timeout.
		request.setRequestHeader( "Content-Type", "application/octet-stream" );
		request.send( method + "\x1f" +
					  urls + "\x1f" +
					  username + "\x1f" +
					  password + "\x1f" +
					  parts + "\x1f" +
					  speed_limit + "\x1f" +
					  directory + "\x1f" +
					  download_operations + "\x1f" +
					  cookies + "\x1f" +
					  headers + "\x1f" +
					  post_data + "\x1f" );
	}
	else
	{
		console.log( "Failed to create XMLHttpRequest." );
	}
}

function DownloadURLS( event )
{
	var add_type = 0;	// Download.
	if ( event.target.id == "add" )
	{
		add_type = 4;	// Add Stopped.
	}

	if ( refresh_server_info )
	{
		browser.runtime.sendMessage(
		{
			type: "server_info"
		} )
		.then( function( info )
		{
			document.getElementById( "server" ).value = info.server;
			document.getElementById( "server_username" ).value = atob( info.username );
			document.getElementById( "server_password" ).value = atob( info.password );

			SendDownloadToClient( add_type );
		} );
	}
	else
	{
		SendDownloadToClient( add_type );
	}
}

function SendPOSTData()
{
	document.getElementById( "post_data" ).disabled = !( document.getElementById( "method" ).checked );
}

function GetDownloadInfo( id )
{
	browser.runtime.sendMessage(
	{
		type: "loading",
		window_id: id
	} )
	.then( function( info )
	{
		document.getElementById( "window_id" ).value = id;
		document.getElementById( "server" ).value = info.server;
		document.getElementById( "server_username" ).value = atob( info.username );
		document.getElementById( "server_password" ).value = atob( info.password );
		document.getElementById( "last_urls" ).value = info.urls;
		document.getElementById( "urls" ).value = info.urls;
		document.getElementById( "cookies" ).value = ( info.cookies ? info.cookies : "" );
		document.getElementById( "headers" ).value = ( info.headers ? info.headers : "" );
		document.getElementById( "parts" ).value = info.parts;
		document.getElementById( "speed_limit" ).value = info.speed_limit;
		document.getElementById( "directory" ).value = ( info.directory ? info.directory : "" );
		var enable_post = ( info.method == 2 ? true : false );
		document.getElementById( "method" ).checked = enable_post;
		document.getElementById( "post_data" ).value = ( info.post_data ? info.post_data : "" );
		document.getElementById( "post_data" ).disabled = !enable_post;

		if ( info.message && info.message != "" )
		{
			window.alert( info.message );
		}
	} );
}

document.addEventListener( "contextmenu", function( e )
{
	if ( e.target.nodeName !== "TEXTAREA" && e.target.nodeName !== "INPUT" || ( e.target.nodeName === "INPUT" && ( e.target.type !== "text" && e.target.type !== "number" && e.target.type !== "password" ) ) )
	{
		e.preventDefault();
	}
}, false );

document.addEventListener( "DOMContentLoaded", function()
{
	browser.windows.getCurrent().then( function( window_info )
	{
		GetDownloadInfo( window_info.id );
	} );

	document.querySelectorAll( "[data-i18n]" ).forEach( el =>
	{
		if ( el.id == "directory" )
		{
			el.placeholder = browser.i18n.getMessage( el.dataset.i18n );
		}
		else
		{
			el.innerText = browser.i18n.getMessage( el.dataset.i18n );
		}
	} );

	document.getElementById( "filter_presets" ).addEventListener( "change", SetFliterPreset );
	document.getElementById( "apply_filter" ).addEventListener( "click", ApplyFliter );
	document.getElementById( "revert_filter" ).addEventListener( "click", RevertFliter );
	document.getElementById( "method" ).addEventListener( "click", SendPOSTData );
	document.getElementById( "advanced" ).addEventListener( "click", AdvancedOptions );
	document.getElementById( "download" ).addEventListener( "click", DownloadURLS );
	document.getElementById( "add" ).addEventListener( "click", DownloadURLS );
	document.getElementById( "cancel" ).addEventListener( "click", CloseWindow );

	g_initial_height = document.body.clientHeight;
	document.body.style.minHeight = g_initial_height + "px";
} );

function SetFliterPreset()
{
	var filter_preset = document.getElementById( "filter_presets" ).value;

	if ( filter_preset == "Images" )
	{
		document.getElementById( "filter" ).value = "^(http|ftpe?)s?:\\/\\/[^\\/\\s]+\\/[^\\?#\\s]+\\.(jp(e?g|e)|gif|png|bmp|tiff?|dib|ico)(\\?|#|$)";
	}
	else if ( filter_preset == "Music" )
	{
		document.getElementById( "filter" ).value = "^(http|ftpe?)s?:\\/\\/[^\\/\\s]+\\/[^\\?#\\s]+\\.(mp3|wave?|flac?|ogg|m4a|wma|aac|midi?|ape|shn|wv|aiff?|oga)(\\?|#|$)";
	}
	else if ( filter_preset == "Videos" )
	{
		document.getElementById( "filter" ).value = "^(http|ftpe?)s?:\\/\\/[^\\/\\s]+\\/[^\\?#\\s]+\\.(avi|mp[124]|m4v|mp(e?g|e)|mkv|webm|wmv|3gp|ogm|ogv|flv|vob)(\\?|#|$)";
	}
	else
	{
		document.getElementById( "filter" ).value = "";
	}
}

function ApplyFliter()
{
	var filter = new RegExp( document.getElementById( "filter" ).value, "i" );

	var urls = document.getElementById( "urls" ).value;
	urls = urls.replace( /\r?\n/g, "\r\n" );
	if ( !urls.endsWith( "\r\n" ) )
	{
		urls += "\r\n";
	}

	document.getElementById( "last_urls" ).value = document.getElementById( "urls" ).value;

	//urls = urls.replace( filter, "" );

	var url_array = urls.split( "\r\n" );
	urls = "";

	var url_count = url_array.length;
	if ( url_count > 0 )
	{
		--url_count;
	}

	for ( var i = 0; i < url_count; ++i )
	{
		if ( filter.test( url_array[ i ] ) )
		{
			urls += url_array[ i ] + "\r\n";
		}
	}

	document.getElementById( "urls" ).value = urls;
}

function RevertFliter()
{
	document.getElementById( "urls" ).value = document.getElementById( "last_urls" ).value;
}

function AdvancedOptions( event )
{
	browser.windows.getCurrent().then( function( window_info )
	{
		var advanced_info = document.getElementById( "advanced_info" );

		if ( event.target.checked )
		{
			advanced_info.style.display = "block";	// Show.

			// The URL and Directory fields + Advanced fields + 15 px top margin for the Advanced fields.
			document.body.style.minHeight = ( g_initial_height + advanced_info.clientHeight + 15 ) + "px";

			if ( window_info.state != "maximized" )
			{
				// The height of the toolbars and borders + the height of all fields.
				var height = ( window_info.height + advanced_info.clientHeight ) + 15;

				browser.windows.update( window_info.id,
				{
					height: height
				} );
			}
		}
		else
		{
			// Need to save the height before we hide the Advanced fields.
			var advanced_info_height = advanced_info.clientHeight;

			advanced_info.style.display = "none";	// Hide.
			
			document.body.style.minHeight = g_initial_height + "px";

			if ( window_info.state != "maximized" )
			{
				// Exclude the Advanced fields + 15 px top margin from the current window height.
				var height = ( window_info.height - advanced_info_height ) - 15;

				browser.windows.update( window_info.id,
				{
					height: height
				} );
			}
		}
	} );
}

function CloseWindow()
{
	var window_id = parseInt( document.getElementById( "window_id" ).value );

	browser.windows.remove( window_id );
}
