var g_open_windows = [];
var g_last_requests = [];
var g_download_info = [];
var g_options = null;

const g_width = 600;
const g_height = 300;
const g_top = Math.round( ( screen.height - g_height ) / 2 );
const g_left = Math.round( ( screen.width - g_width ) / 2 );

function OnGetOptions( options )
{
	if ( typeof options.server == "undefined" ) { options.server = "http://localhost:80/"; }
	if ( typeof options.username == "undefined" ) { options.username = ""; }
	if ( typeof options.password == "undefined" ) { options.password = ""; }
	if ( typeof options.user_agent == "undefined" ) { options.user_agent = true; }
	if ( typeof options.referer == "undefined" ) { options.referer = true; }
	if ( typeof options.override == "undefined" ) { options.override = false; }
	if ( typeof options.override_prompts == "undefined" ) { options.override_prompts = true; }
	if ( typeof options.show_add_window == "undefined" ) { options.show_add_window = false; }
	if ( typeof options.from_current_domain == "undefined" ) { options.from_current_domain = false; }

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
		var method = download_info.method;
		var url = download_info.url;
		var cookie_string = download_info.cookie_string;
		var headers = download_info.headers;
		var post_data = download_info.post_data;
		var directory = download_info.directory;
		var filename = download_info.filename;

		g_open_windows.push(
		[
			window_info.id,
			method,
			url,
			cookie_string,
			headers,
			post_data,
			directory,
			filename,
			message
		] );
	} );
}

function OnGetCookieString( cookies )
{
	var cookie_string = "";

	if ( typeof cookies != "undefined" && cookies.length > 0 )
	{
		cookie_string = cookies[ 0 ].name + "=" + cookies[ 0 ].value;

		for ( var i = 1; i < cookies.length; ++i )
		{
			cookie_string += "; " + cookies[ i ].name + "=" + cookies[ i ].value;
		}
	}

	return cookie_string;
}

function OnGetCookieDomain( cookies )
{
	var cookie_domain = "";

	if ( typeof cookies != "undefined" && cookies.length > 0 )
	{
		cookie_domain = cookies[ 0 ].domain;

		for ( var i = 1; i < cookies.length; ++i )
		{
			if ( cookies[ i ].domain.length < cookie_domain.length )
			{
				cookie_domain = cookies[ i ].domain;
			}
		}
	}

	return cookie_domain;
}

// Recursively find a cookie string in our cookie stores.
function GetCookies( cookie_info, download_info )
{
	var request;

	if ( typeof cookie_info.domain != "undefined" )
	{
		request = { domain: cookie_info.domain, storeId: cookie_info.cookie_stores[ cookie_info.index ].id };
	}
	else
	{
		request = { url: cookie_info.url, storeId: cookie_info.cookie_stores[ cookie_info.index ].id };
	}

	chrome.cookies.getAll( request, function( cookies )
	{
		var cookie_string = OnGetCookieString( cookies );

		// No cookie string? Look in the next cookie store.
		if ( cookie_string == "" && ( ++cookie_info.index < cookie_info.cookie_stores.length ) )
		{
			GetCookies( cookie_info, download_info );
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

function GetCookieDomain( cookie_info, download_info )
{
	chrome.cookies.getAll( { url: cookie_info.url, storeId: cookie_info.cookie_stores[ cookie_info.index ].id }, function( cookies )
	{
		var domain = OnGetCookieDomain( cookies );

		// No cookie domain? Look in the next cookie store.
		if ( domain == "" && ( ++cookie_info.index < cookie_info.cookie_stores.length ) )
		{
			GetCookieDomain( cookie_info, download_info );
		}
		else	// We've exhausted all cookie stores, or we've found a domain.
		{
			cookie_info.domain = domain;
			GetCookies( cookie_info, download_info );
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
					method: window[ 1 ],
					urls: window[ 2 ],
					cookies: window[ 3 ],
					headers: window[ 4 ],
					post_data: window[ 5 ],
					directory: window[ 6 ],
					filename: window[ 7 ],
					message: window[ 8 ]
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
		var override_prompts = g_options.override_prompts;

		sendResponse(
		{
			server: server,
			username: username,
			password: password,
			override_prompts: override_prompts
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

			chrome.browserAction.setIcon( ( g_options.override ? { path: "icons/icon-e-64.png" } : { path: "icons/icon-d-64.png" } ) );
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
		var download_operations = ( g_options.override_prompts ? 2 : 0 );
		var url;

		if ( download_info.filename != "" )
		{
			url = "[" + download_info.filename + "]" + download_info.url;
		}
		else
		{
			url = download_info.url;
		}

		request.onerror = function( e )
		{
			CreateDownloadWindow( download_info, chrome.i18n.getMessage( "SEND_FAILED" ) );
		};

		request.ontimeout = function( e )
		{
			CreateDownloadWindow( download_info, chrome.i18n.getMessage( "CONNECTION_TIMEOUT" ) );
		};

		request.onload = function( e )
		{
			if ( request.responseText != "DOWNLOADING" )
			{
				CreateDownloadWindow( download_info, chrome.i18n.getMessage( "INVALID_RESPONSE" ) );
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
					  url + "\x1f" +
					  download_info.directory + "\x1f" +
					  "\x1f" +									// Parts
					  "\x1f" +									// SSL/TLS version
					  "\x1f" +									// Username
					  "\x1f" +									// Password
					  "\x1f" +									// Speed limit
					  download_operations + "\x1f" +
					  download_info.cookie_string + "\x1f" +
					  download_info.headers + "\x1f" +
					  download_info.post_data + "\x1f" +
					  "\x1f" +									// Proxy type
					  "\x1f" +									// Proxy hostname/IP address
					  "\x1f" +									// Proxy port
					  "\x1f" +									// Proxy username
					  "\x1f" +									// Proxy password
					  "\x1f" +									// Proxy resolve domain names
					  "\x1f"									// Proxy use authentication
					);
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
		GetCookies( { url: download_info.url, cookie_stores: cookie_stores, index: 0 }, download_info );
	} );
}

function GetDownloadInfoIndex( id )
{
	for ( var i = 0; i < g_download_info.length; ++i )
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
			var filepath_length = item.filename.current.lastIndexOf( "\\" );
			download_info.filename = item.filename.current.substring( filepath_length + 1 );
			download_info.directory = item.filename.current.substring( 0, filepath_length );
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
		var url = ( item.hasOwnProperty( "finalUrl" ) ? item.finalUrl : item.url );
		var protocol = url.substring( 0, 8 ).toLowerCase();

		if ( protocol.startsWith( "http:" ) || protocol.startsWith( "https:" ) || protocol.startsWith( "ftp:" ) )
		{
			var method = 1; // GET
			var post_data = "";

			var request_body = GetPOSTRequestBody( item.url );

			if ( request_body != null )
			{
				method = 2; // POST

				// Format the POST data as a URL encoded string.
				if ( request_body.formData != null )
				{
					var values = Object.entries( request_body.formData );
					post_data = values[ 0 ][ 0 ] + "=" + values[ 0 ][ 1 ];

					for ( var i = 1; i < values.length; ++i )
					{
						post_data += "&" + values[ i ][ 0 ] + "=" + values[ i ][ 1 ];
					}
				}
			}

			var id = item.id;
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
									directory: "",
									filename: "",
									show_add_window: show_add_window } );
		}
	}
}

function GetPOSTRequestBody( url )
{
	var request = null;

	for ( var i = 0; i < g_last_requests.length; ++i )
	{
		if ( g_last_requests[ i ][ 0 ] == url )
		{
			request = g_last_requests[ i ][ 1 ];
		}
	}

	g_last_requests.length = 0;	// Clear the array of requests.

	return request;
}

function GetURLRequest( request )
{
	if ( request.method == "POST" )
	{
		g_last_requests.unshift( [ request.url, request.requestBody ] );

		if ( g_last_requests.length > 10 )
		{
			g_last_requests.pop();
		}
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
			if ( typeof urls == "undefined" )
			{
				urls = "";
			}

			var new_urls = "";
			var get_cookies = true;

			if ( urls.length > 0 )
			{
				var page_hostname = new URL( info.pageUrl ).hostname;
				var last_hostname = null;

				var url_array = urls[ 0 ].split( "\r\n" );
				for ( var i = 0; i < url_array.length; ++i )
				{
					if ( url_array[ i ] != "" )
					{
						var hostname = new URL( url_array[ i ] ).hostname;
						if ( hostname != "" )
						{
							if ( g_options.from_current_domain )
							{
								if ( hostname == page_hostname )
								{
									new_urls += url_array[ i ] + "\r\n"
								}
							}
							else
							{
								if ( last_hostname == null )
								{
									last_hostname = hostname;
								}
								else if ( last_hostname != hostname )
								{
									get_cookies = false;

									break;
								}
							}
						}
					}
				}
			}

			if ( new_urls != "" )
			{
				urls = new_urls;
			}

			if ( get_cookies )
			{
				chrome.cookies.getAllCookieStores( function( cookie_stores )
				{
					GetCookieDomain( { url: info.pageUrl, cookie_stores: cookie_stores, index: 0 },
									 { show_add_window: true, id: null, method: "1", url: urls, cookie_string: "", headers: headers, directory: "", filename: "", post_data: "" } );
				} );
			}
			else
			{
				CreateDownloadWindow( { show_add_window: true, id: null, method: "1", url: urls, cookie_string: "", headers: headers, directory: "", filename: "", post_data: "" } );
			}
		} );
	}
	else if ( info.menuItemId == "from-current-domain" )
	{
		g_options.from_current_domain = !g_options.from_current_domain;

		chrome.storage.local.set( { from_current_domain: g_options.from_current_domain } );
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
			GetCookies( { url: url, cookie_stores: cookie_stores, index: 0 },
						{ show_add_window: true, id: null, method: "1", url: url, cookie_string: "", headers: headers, directory: "", filename: "", post_data: "" } );
		} );
	}
}

function HandleCommand( command )
{
	if ( command === "override_download_manager" )
	{
		g_options.override = !g_options.override;

		chrome.storage.local.set(
		{
			override: g_options.override
		}, function()
		{
			HandleMessages( { type: "refresh_options" }, null, ()=>{} );
		} );
	}
	else if ( command === "add_urls_window" )
	{
		CreateDownloadWindow( { show_add_window: true, id: null, method: "1", url: "", cookie_string: "", headers: "", directory: "", filename: "", post_data: "" } );
	}
}

chrome.storage.local.get( null, function( options )
{
	g_options = OnGetOptions( options );

	chrome.contextMenus.create(
	{
		id: "download-link",
		title: chrome.i18n.getMessage( "menu_download_link" ),
		contexts: [ "link" ]
	} );

	chrome.contextMenus.create(
	{
		id: "download-image",
		title: chrome.i18n.getMessage( "menu_download_image" ),
		contexts: [ "image" ]
	} );

	chrome.contextMenus.create(
	{
		id: "download-audio",
		title: chrome.i18n.getMessage( "menu_download_audio" ),
		contexts: [ "audio" ]
	} );

	chrome.contextMenus.create(
	{
		id: "download-video",
		title: chrome.i18n.getMessage( "menu_download_video" ),
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
		title: chrome.i18n.getMessage( "menu_download_all_images" ),
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	chrome.contextMenus.create(
	{
		id: "download-all-media",
		title: chrome.i18n.getMessage( "menu_download_all_media" ),
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	chrome.contextMenus.create(
	{
		id: "download-all-links",
		title: chrome.i18n.getMessage( "menu_download_all_links" ),
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	chrome.contextMenus.create(
	{
		id: "from-current-domain",
		title: chrome.i18n.getMessage( "menu_from_current_domain" ),
		type: "checkbox",
		checked: g_options.from_current_domain,
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
		title: chrome.i18n.getMessage( "menu_download_page" ),
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	chrome.contextMenus.onClicked.addListener( OnMenuClicked );

	chrome.runtime.onMessage.addListener( HandleMessages );

	chrome.commands.onCommand.addListener( HandleCommand );

	if ( g_options.override )
	{
		chrome.browserAction.setIcon( { path: "icons/icon-e-64.png" } );
		chrome.webRequest.onBeforeRequest.addListener( GetURLRequest, { urls: [ "<all_urls>" ] }, [ "requestBody" ] );
		chrome.downloads.onCreated.addListener( OnDownloadItemCreated );
		chrome.downloads.onChanged.addListener( OnDownloadItemChange );
	}
} );
