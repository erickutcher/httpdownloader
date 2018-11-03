var g_open_windows = [];
var last_request = null;
var g_download_info = [];
var g_options = null;

const g_top = Math.round( ( screen.height - 295 ) / 2 );
const g_left = Math.round( ( screen.width - 640 ) / 2 );
const g_width = 640;
const g_height = 295;

function GetDomain( url )
{
	var parsed_url = document.createElement( "a" );
	parsed_url.href = url;
	var domain = parsed_url.hostname;
	var domain_parts = domain.split( "." );
	if ( domain_parts.length > 2 )
	{
		domain = domain_parts[ domain_parts.length - 2 ] + "." + domain_parts[ domain_parts.length - 1 ];
	}

	return domain;
}

function OnGetOptions( options )
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

	return options;
}

function CreateDownloadWindow( download_info, message = "" )
{
	chrome.windows.create(
	{
		url: chrome.extension.getURL( "download.html" ),
		type: "popup",
		left: g_left,
		top: g_top,
		width: g_width,
		height: g_height
	}, function( window_info )
	{
		var server = g_options.server;
		var username = g_options.username;
		var password = g_options.password;
		var parts = g_options.parts;

		var method = download_info.method;
		var url = download_info.url;
		var cookie_string = download_info.cookie_string;
		var headers = download_info.headers;
		var post_data = download_info.post_data;
		var directory = download_info.directory;

		g_open_windows.push(
		[
			window_info.id,
			server,
			username,
			password,
			parts,
			method,
			url,
			cookie_string,
			headers,
			post_data,
			directory,
			message
		] );
	} );
}

function OnGetCookieString( cookies )
{
	var cookie_string = "";
	var cookies_length = cookies.length;

	if ( cookies_length > 0 )
	{
		cookie_string = cookies[ 0 ].name + "=" + cookies[ 0 ].value;

		for ( var i = 1; i < cookies_length; ++i )
		{
			cookie_string += "; " + cookies[ i ].name + "=" + cookies[ i ].value;
		}
	}

	return cookie_string;
}

// Recursively find a cookie string in our cookie stores.
function GetCookies( cookie_info, download_info )
{
	chrome.cookies.getAll( { domain: "." + cookie_info.domain, storeId: cookie_info.cookie_stores[ cookie_info.index ].id }, function( cookies )
	{
		var cookie_string = OnGetCookieString( cookies );

		// No cookie string? Look in the next cookie store.
		if ( cookie_string == "" && ( ++cookie_info.index < cookie_info.cookie_stores.length ) )
		{
			GetCookies( cookie_info, download_info )
		}
		else	// We've exhausted all cookie stores, or we've found a string.
		{
			download_info.cookie_string = cookie_string;

			if ( download_info.show_add_window )
			{
				CreateDownloadWindow( download_info );
			}
			else
			{
				SendDownloadToClient( download_info );
			}

			if ( download_info.id != null )
			{
				// Erase it from the download manager's history.
				chrome.downloads.erase( { id: download_info.id } );
			}
		}
	} );
}

function HandleMessages( request, sender, sendResponse )
{
	if ( request.type == "loading" )
	{
		for ( var i = 0; i < g_open_windows.length; ++i )
		{
			if ( g_open_windows[ i ][ 0 ] == request.window_id )
			{
				var window = g_open_windows[ i ];
				g_open_windows.splice( i, 1 );

				sendResponse(
				{
					server: window[ 1 ],
					username: window[ 2 ],
					password: window[ 3 ],
					parts: window[ 4 ],
					method: window[ 5 ],
					urls: window[ 6 ],
					cookies: window[ 7 ],
					headers: window[ 8 ],
					post_data: window[ 9 ],
					directory: window[ 10 ],
					message: window[ 11 ]
				} );

				break;
			}
		}
	}
	else if ( request.type == "server_info" )
	{
		var server = g_options.server;
		var username = g_options.username;
		var password = g_options.password;

		sendResponse(
		{
			server: server,
			username: username,
			password: password
		} );
	}
	else if ( request.type == "refresh_options" )
	{
		chrome.storage.local.get( null, function( options )
		{
			g_options = OnGetOptions( options );

			if ( chrome.webRequest.onBeforeRequest.hasListener( GetURLRequest ) )
			{
				if ( !g_options.override )
				{
					chrome.webRequest.onBeforeRequest.removeListener( GetURLRequest );
				}
			}
			else
			{
				if ( g_options.override )
				{
					chrome.webRequest.onBeforeRequest.addListener( GetURLRequest, { urls: [ "<all_urls>" ] }, [ "requestBody" ] );
				}
			}

			if ( chrome.downloads.onCreated.hasListener( OnDownloadItemCreated ) )
			{
				if ( !g_options.override )
				{
					chrome.downloads.onCreated.removeListener( OnDownloadItemCreated );
				}
			}
			else
			{
				if ( g_options.override )
				{
					chrome.downloads.onCreated.addListener( OnDownloadItemCreated );
				}
			}

			if ( chrome.downloads.onChanged.hasListener( OnDownloadItemChange ) )
			{
				if ( !g_options.override )
				{
					chrome.downloads.onChanged.removeListener( OnDownloadItemChange );
				}
			}
			else
			{
				if ( g_options.override )
				{
					chrome.downloads.onChanged.addListener( OnDownloadItemChange );
				}
			}
		} );

		sendResponse( {} );
	}

	return true;
}

function SendDownloadToClient( download_info )
{
	var request = new XMLHttpRequest();
	if ( request )
	{
		var server = g_options.server;
		var server_username = g_options.server;
		var server_password = g_options.password;
		var username = "";
		var password = "";
		var parts = g_options.parts;
		var simulate_download = "0";

		request.onerror = function( e )
		{
			//console.log( "An error occurred while sending the download request." );
			CreateDownloadWindow( download_info, "An error occurred while sending the download request." );
		};

		request.ontimeout = function( e )
		{
			//console.log( "The connection has timed out while sending the download request." );
			CreateDownloadWindow( download_info, "The connection has timed out while sending the download request." );
		};

		request.onload = function( e )
		{
			if ( request.responseText != "DOWNLOADING" )
			{
				//console.log( "The server returned an invalid response to our download request." );
				CreateDownloadWindow( download_info, "The server returned an invalid response to our download request." );
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
		request.send( download_info.method + "\x1f" +
					  download_info.url + "\x1f" +
					  username + "\x1f" +
					  password + "\x1f" +
					  parts + "\x1f" +
					  download_info.directory + "\x1f" +
					  simulate_download + "\x1f" +
					  download_info.cookie_string + "\x1f" +
					  download_info.headers + "\x1f" +
					  download_info.post_data + "\x1f" );
	}
	else
	{
		console.log( "Failed to create XMLHttpRequest." );
	}
}

function InitializeDownload( download_info )
{
	// Need to go through each cookie store if we're incognito/private browsing. Dumb!
	chrome.cookies.getAllCookieStores( function( cookie_stores )
	{
		var domain = GetDomain( download_info.url );

		GetCookies( { domain: domain, cookie_stores: cookie_stores, index: 0 }, download_info );
	} );
}

function GetDownloadInfoIndex( id )
{
	for ( i = 0; i < g_download_info.length; ++i )
	{
		if ( g_download_info[ i ].id == id )
		{
			return i;
		}
	}

	return null;
}

// We don't get the filename right away.
// We'll need to wait for it and also keep track of other downloads that might occur.
function OnDownloadItemChange( item )
{
	// Make sure we're handling a captured download.
	var index = GetDownloadInfoIndex( item.id );
	if ( index == null )
	{
		return;
	}

	var download_info = g_download_info[ index ];

	// Try to get a directory and then cancel the download if it's found.
	if ( item.filename && !download_info.have_everything )
	{
		if ( item.filename.current != "" )
		{
			download_info.directory = item.filename.current.substring( 0, item.filename.current.lastIndexOf( "\\" ) );
		}

		download_info.have_everything = true;

		// Cancel the download before it begins.
		chrome.downloads.cancel( item.id );
	}
	else if ( item.state )
	{
		// Remove our download info from the global array.
		g_download_info.splice( index, 1 );

		// Only do the download if we have everything we need.
		if ( download_info.have_everything )
		{
			if ( item.state.current == "complete" )
			{
				// Remove it from the disk if it exists.
				chrome.downloads.removeFile( item.id, function()
				{
					// Transfer the download to our client.
					InitializeDownload( download_info );
				} );
			}
			else	// "interrupted" or "in_progress"
			{
				// Transfer the download to our client.
				InitializeDownload( download_info );
			}
		}
	}
}

function OnDownloadItemCreated( item )
{
	// Do we want to handle the download management?
	if ( g_options.override )
	{
		var method = 1; // GET
		var post_data = "";

		if ( last_request != null && last_request.url == item.url )
		{
			method = 2; // POST

			// Format the POST data as a URL encoded string.
			if ( last_request.requestBody != null && last_request.requestBody.formData != null )
			{
				var values = Object.entries( last_request.requestBody.formData );
				post_data = values[ 0 ][ 0 ] + "=" + values[ 0 ][ 1 ];

				for ( var i = 1; i < values.length; ++i )
				{
					post_data += "&" + values[ i ][ 0 ] + "=" + values[ i ][ 1 ];
				}
			}
		}

		last_request = null;

		var id = item.id;
		var url = item.url;
		var directory = g_options.default_directory;
		var show_add_window = g_options.show_add_window;

		var headers = "";

		if ( g_options.user_agent )
		{
			headers = "User-Agent: " + window.navigator.userAgent + "\r\n";
		}

		if ( g_options.referer && item.referrer != null && item.referrer != "" )
		{
			headers += "Referer: " + item.referrer + "\r\n";
		}

		// Add our download info to the global array and then wait for a filename in chrome.downloads.onChanged.
		g_download_info.push( { have_everything: false,
								id: id,
								method: method,
								url: url,
								cookie_string: "",
								headers: headers,
								post_data: post_data,
								directory: directory,
								show_add_window: show_add_window } );
	}
}

function GetURLRequest( request )
{
	if ( request.method == "POST" )
	{
		last_request = request;
	}
	else
	{
		last_request = null;
	}
}

function OnMenuClicked( info, tab )
{
	var headers = "";

	if ( g_options.user_agent )
	{
		headers = "User-Agent: " + window.navigator.userAgent + "\r\n";
	}

	if ( g_options.referer && info.pageUrl != null )
	{
		headers += "Referer: " + info.pageUrl.split( '#' )[ 0 ] + "\r\n";
	}

	if ( info.menuItemId == "download-all-images" ||
		 info.menuItemId == "download-all-media" ||
		 info.menuItemId == "download-all-links" )
	{
		var script_file = "";

		if ( info.menuItemId == "download-all-images" )
		{
			script_file = "get_images.js"
		}
		else if ( info.menuItemId == "download-all-media" )
		{
			script_file = "get_media.js"
		}
		else
		{
			script_file = "get_links.js"
		}

		chrome.tabs.executeScript( { file: script_file }, function( urls )
		{
			var directory = g_options.default_directory;

			CreateDownloadWindow( { show_add_window: true, id: null, method: "1", url: urls, cookie_string: "", headers: headers, directory: directory, post_data: "" } );
		} );
	}
	else
	{
		var url = "";

		if ( info.menuItemId == "download-link" )
		{
			url = info.linkUrl;
		}
		else if ( info.menuItemId == "download-image" ||
				  info.menuItemId == "download-audio" ||
				  info.menuItemId == "download-video" )
		{
			url = info.srcUrl;
		}
		else
		{
			url = info.pageUrl;
		}

		// Need to go through each cookie store if we're incognito/private browsing. Dumb!
		chrome.cookies.getAllCookieStores( function( cookie_stores )
		{
			var domain = GetDomain( url );
			var directory = g_options.default_directory;

			GetCookies( { domain: domain, cookie_stores: cookie_stores, index: 0 },
						{ show_add_window: true, id: null, method: "1", url: url, cookie_string: "", headers: headers, directory: directory, post_data: "" } );
		} );
	}
}

chrome.storage.local.get( null, function( options )
{
	g_options = OnGetOptions( options );

	chrome.contextMenus.create(
	{
		id: "download-link",
		title: "Download Link...",
		contexts: [ "link" ]
	} );

	chrome.contextMenus.create(
	{
		id: "download-image",
		title: "Download Image...",
		contexts: [ "image" ]
	} );

	chrome.contextMenus.create(
	{
		id: "download-audio",
		title: "Download Audio...",
		contexts: [ "audio" ]
	} );

	chrome.contextMenus.create(
	{
		id: "download-video",
		title: "Download Video...",
		contexts: [ "video" ]
	} );

	chrome.contextMenus.create(
	{
		id: "separator-1",
		type: "separator",
		contexts: [ "link", "image", "audio", "video" ]
	} );

	chrome.contextMenus.create(
	{
		id: "download-all-images",
		title: "Download All Images...",
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	chrome.contextMenus.create(
	{
		id: "download-all-media",
		title: "Download All Media...",
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	chrome.contextMenus.create(
	{
		id: "download-all-links",
		title: "Download All Links...",
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	chrome.contextMenus.create(
	{
		id: "separator-2",
		type: "separator",
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	chrome.contextMenus.create(
	{
		id: "download-page",
		title: "Download Page...",
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	chrome.contextMenus.onClicked.addListener( OnMenuClicked );

	chrome.runtime.onMessage.addListener( HandleMessages );

	if ( g_options.override )
	{
		chrome.webRequest.onBeforeRequest.addListener( GetURLRequest, { urls: [ "<all_urls>" ] }, [ "requestBody" ] );
		chrome.downloads.onCreated.addListener( OnDownloadItemCreated );
		chrome.downloads.onChanged.addListener( OnDownloadItemChange );
	}
} );
