var refresh_server_info = false;

function BeginDownload()
{
	var request = new XMLHttpRequest();
	if ( request )
	{
		var window_id = parseInt( document.getElementById( "window_id" ).value );
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
		var simulate_download = ( document.getElementById( "simulate_download" ).checked ? "1" : "0" );

		var download = document.getElementById( "download" );
		download.disabled = true;

		request.onerror = function( e )
		{
			refresh_server_info = true;

			download.disabled = false;

			window.alert( "An error occurred while sending the download request." );
		};

		request.ontimeout = function( e )
		{
			refresh_server_info = true;

			download.disabled = false;

			window.alert( "The connection has timed out while sending the download request." );
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

				window.alert( "The server returned an invalid response to our download request." );
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
		request.send( urls + "\x1f" + username + "\x1f" + password + "\x1f" + parts + "\x1f" + simulate_download + "\x1f" + cookies + "\x1f" + headers + "\x1f" );
	}
	else
	{
		console.log( "Failed to create XMLHttpRequest." );
	}
}

function DownloadURLS()
{
	if ( refresh_server_info )
	{
		browser.runtime.sendMessage(
		{
			type: "server_info"
		} )
		.then( function( info )
		{
			if ( info )
			{
				document.getElementById( "server" ).value = info.server;
				document.getElementById( "server_username" ).value = atob( info.username );
				document.getElementById( "server_password" ).value = atob( info.password );

				BeginDownload();
			}
		} );
	}
	else
	{
		BeginDownload();
	}
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
		if ( info )
		{
			document.getElementById( "window_id" ).value = id;
			document.getElementById( "server" ).value = info.server;
			document.getElementById( "server_username" ).value = atob( info.username );
			document.getElementById( "server_password" ).value = atob( info.password );
			document.getElementById( "last_urls" ).value = info.urls;
			document.getElementById( "urls" ).value = info.urls;
			document.getElementById( "cookies" ).value = ( info.cookies ? info.cookies : "" );
			document.getElementById( "parts" ).value = info.parts;
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
	if ( browser.windows )
	{
		browser.windows.getCurrent().then( function( window_info )
		{
			if ( window_info )
			{
				GetDownloadInfo( window_info.id );
			}
		} );

		document.getElementById( "filter_presets" ).addEventListener( "change", SetFliterPreset );
		document.getElementById( "apply_filter" ).addEventListener( "click", ApplyFliter );
		document.getElementById( "revert_filter" ).addEventListener( "click", RevertFliter );
		document.getElementById( "advanced" ).addEventListener( "click", AdvancedOptions );
		document.getElementById( "download" ).addEventListener( "click", DownloadURLS );
		document.getElementById( "cancel" ).addEventListener( "click", CloseWindow );
	}
} );

function SetFliterPreset()
{
	var filter_preset = document.getElementById( "filter_presets" ).value;

	if ( filter_preset == "Images" )
	{
		//document.getElementById( "filter" ).value = "https?:\\/\\/[^\\/\\s]+\\/(?!\\S+\\.(jpe?g|jpe|gif|png|bmp|tiff?|dib|ico)(\\?|#|\\r?\\n)).*\\r?\\n";
		document.getElementById( "filter" ).value = "^https?:\\/\\/[^\\/\\s]+\\/[^\\?#\\s]+\\.(jpe?g|jpe|gif|png|bmp|tiff?|dib|ico)(\\?|#|$)";
	}
	else if ( filter_preset == "Music" )
	{
		//document.getElementById( "filter" ).value = "https?:\\/\\/[^\\/\\s]+\\/(?!\\S+\\.(mp3|wave?|flac?|ogg|m4a|wma|aac|midi?|ape|shn|wv|aiff?|oga)(\\?|#|\\r?\\n)).*\\r?\\n";
		document.getElementById( "filter" ).value = "^https?:\\/\\/[^\\/\\s]+\\/[^\\?#\\s]+\\.(mp3|wave?|flac?|ogg|m4a|wma|aac|midi?|ape|shn|wv|aiff?|oga)(\\?|#|$)";
	}
	else if ( filter_preset == "Videos" )
	{
		//document.getElementById( "filter" ).value = "https?:\\/\\/[^\\/\\s]+\\/(?!\\S+\\.(avi|mp4|m4v|mp2|mp1|mpe?g|mpe|mkv|webm|wmv|3gp|ogm|ogv|flv|vob)(\\?|#|\\r?\\n)).*\\r?\\n";
		document.getElementById( "filter" ).value = "^https?:\\/\\/[^\\/\\s]+\\/[^\\?#\\s]+\\.(avi|mp4|m4v|mp2|mp1|mpe?g|mpe|mkv|webm|wmv|3gp|ogm|ogv|flv|vob)(\\?|#|$)";
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

function AdvancedOptions()
{
	var advanced = document.getElementById( "advanced" );
	var advanced_info = document.getElementById( "advanced_info" );

	if ( advanced_info.style.display == "" )
	{
		advanced.value = "Advanced \xBB";
		advanced_info.style.display = "none";	// Hide.
	}
	else
	{
		browser.windows.getCurrent().then( function( window_info )
		{
			if ( window_info && window_info.height < 600 )
			{
				browser.windows.update( window_info.id,
				{
					height: 600
				} );
			}
		} );

		advanced.value = "Advanced \xAB";
		advanced_info.style.display = "";	// Show.
	}
}

function CloseWindow()
{
	var window_id = parseInt( document.getElementById( "window_id" ).value );

	browser.windows.remove( window_id );
}
