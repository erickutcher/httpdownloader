var g_open_windows = [];
var g_last_requests = [];
var g_handled_windows = [];
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
	if ( typeof options.theme_support == "undefined" ) { options.theme_support = true; }
	if ( typeof options.override_prompts == "undefined" ) { options.override_prompts = true; }
	if ( typeof options.show_add_window == "undefined" ) { options.show_add_window = false; }

	return options;
}

function CreateDownloadWindow( download_info, message = "" )
{
	browser.windows.create(
	{
		url: browser.runtime.getURL( "download.html" ),
		type: "popup",
		left: g_left,
		top: g_top,
		width: g_width,
		height: g_height
	} )
	.then( function( window_info )
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
			window_info.tabs[ 0 ].id,
			g_options.theme_support,
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

	if ( cookie_info.domain != "" )
	{
		request = { domain: cookie_info.domain, storeId: cookie_info.cookie_stores[ cookie_info.index ].id };
	}
	else
	{
		request = { url: cookie_info.url, storeId: cookie_info.cookie_stores[ cookie_info.index ].id };
	}

	browser.cookies.getAll( request ).then( OnGetCookieString )
	.then( function( cookie_string )
	{
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
				browser.downloads.erase( { id: download_info.id } );
			}
		}
	} );
}

function GetCookieDomain( cookie_info, download_info )
{
	browser.cookies.getAll( { url: cookie_info.url, storeId: cookie_info.cookie_stores[ cookie_info.index ].id } ).then( OnGetCookieDomain )
	.then( function( domain )
	{
		// No cookie domain? Look in the next cookie store.
		if ( domain == "" && ( ++cookie_info.index < cookie_info.cookie_stores.length ) )
		{
			GetCookieDomain( cookie_info, download_info );
		}
		else	// We've exhausted all cookie stores, or we've found a domain.
		{
			if ( cookie_info.index >= cookie_info.cookie_stores.length )
			{
				cookie_info.index = 0;
			}
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
				g_open_windows.splice( i, 2 );

				sendResponse(
				{
					theme_support: window[ 2 ],
					method: window[ 3 ],
					urls: window[ 4 ],
					cookies: window[ 5 ],
					headers: window[ 6 ],
					post_data: window[ 7 ],
					directory: window[ 8 ],
					filename: window[ 9 ],
					message: window[ 10 ]
				} );

				g_handled_windows.push( { window_id: window[ 0 ], tab_id: window[ 1 ] } );

				break;
			}
		}
	}
	else if ( request.type == "close_window" )
	{
		for ( var i = 0; i < g_handled_windows.length; ++i )
		{
			var window_id;
			if ( g_handled_windows[ i ] == request.window_id )
			{
				window_id = g_handled_windows[ i ];
				g_handled_windows.splice( i, 1 );
			}

			sendResponse( { window_id: window_id } );
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
		browser.storage.local.get().then( OnGetOptions )
		.then( function( options )
		{
			var theme_support = g_options.theme_support;

			g_options = options;

			if ( browser.webRequest.onBeforeRequest.hasListener( GetURLRequest ) )
			{
				if ( !g_options.override )
				{
					browser.webRequest.onBeforeRequest.removeListener( GetURLRequest );
				}
			}
			else
			{
				if ( g_options.override )
				{
					browser.webRequest.onBeforeRequest.addListener( GetURLRequest, { urls: [ "<all_urls>" ] }, [ "requestBody" ] );
				}
			}

			if ( browser.webRequest.onBeforeSendHeaders.hasListener( GetURLHeaders ) )
			{
				if ( !g_options.override )
				{
					browser.webRequest.onBeforeSendHeaders.removeListener( GetURLHeaders );
				}
			}
			else
			{
				if ( g_options.override )
				{
					browser.webRequest.onBeforeSendHeaders.addListener( GetURLHeaders, { urls: [ "<all_urls>" ] }, [ "requestHeaders" ] );
				}
			}

			if ( browser.downloads.onCreated.hasListener( OnDownloadItemCreated ) )
			{
				if ( !g_options.override )
				{
					browser.downloads.onCreated.removeListener( OnDownloadItemCreated );
				}
			}
			else
			{
				if ( g_options.override )
				{
					browser.downloads.onCreated.addListener( OnDownloadItemCreated );
				}
			}

			if ( g_options.theme_support != theme_support )
			{
				for ( var i = 0; i < g_handled_windows.length; ++i )
				{
					browser.tabs.sendMessage( g_handled_windows[ i ].tab_id, { type: "theme_support", value: g_options.theme_support } );
				}
			}

			browser.browserAction.setIcon( ( g_options.override ? { path: "icons/icon-e-64.png" } : { path: "icons/icon-d-64.png" } ) );
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
		var server_username = atob( g_options.username );
		var server_password = atob( g_options.password );
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
			CreateDownloadWindow( download_info, browser.i18n.getMessage( "SEND_FAILED" ) );
		};

		request.ontimeout = function( e )
		{
			CreateDownloadWindow( download_info, browser.i18n.getMessage( "CONNECTION_TIMEOUT" ) );
		};

		request.onload = function( e )
		{
			if ( request.responseText != "DOWNLOADING" )
			{
				CreateDownloadWindow( download_info, browser.i18n.getMessage( "INVALID_RESPONSE" ) );
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
	browser.cookies.getAllCookieStores()
	.then( function( cookie_stores )
	{
		GetCookies( { url: download_info.url, cookie_stores: cookie_stores, index: 0, domain: "" }, download_info );
	} );
}

function OnDownloadItemCreated( item )
{
	// Do we want to handle the download management?
	if ( g_options.override )
	{
		var protocol = item.url.substring( 0, 8 ).toLowerCase();

		if ( protocol.startsWith( "https:" ) ||
			 protocol.startsWith( "http:" ) ||
			 protocol.startsWith( "ftp:" ) ||
			 protocol.startsWith( "sftp:" ) )
		{
			var method = 1; // GET
			var headers = "";
			var post_data = "";

			var request_info = GetPOSTRequestInfo( item.url, 2 );

			if ( request_info != null )
			{
				method = 2; // POST

				// Format the POST data as a URL encoded string.
				if ( request_info.formData != null )
				{
					var values = Object.entries( request_info.formData );
					post_data = values[ 0 ][ 0 ] + "=" + values[ 0 ][ 1 ];

					for ( var i = 1; i < values.length; ++i )
					{
						post_data += "&" + values[ i ][ 0 ] + "=" + values[ i ][ 1 ];
					}

					request_info = GetPOSTRequestInfo( item.url, 3 );

					if ( request_info != null )
					{
						headers = "Content-Type: " + request_info + "\r\n";
					}
				}
			}

			g_last_requests.length = 0;	// Clear the array of requests.

			// Cancel the download before it begins.
			browser.downloads.cancel( item.id )
			.then ( function()
			{
				var directory;
				var filename;

				if ( item.filename != "" )
				{
					var filepath_length = item.filename.lastIndexOf( "\\" );
					filename = item.filename.substring( filepath_length + 1 );
					directory = item.filename.substring( 0, filepath_length );
				}
				else
				{
					filename = "";
					directory = "";
				}

				var url = item.url;
				var id = item.id;
				var show_add_window = g_options.show_add_window;

				if ( g_options.user_agent )
				{
					headers += "User-Agent: " + window.navigator.userAgent + "\r\n";
				}

				if ( g_options.referer && item.referrer != null && item.referrer != "" )
				{
					headers += "Referer: " + item.referrer + "\r\n";
				}

				var download_info = { id: id,
									  method: method,
									  url: url,
									  cookie_string: "",
									  headers: headers,
									  post_data: post_data,
									  directory: directory,
									  filename: filename,
									  show_add_window: show_add_window };

				if ( item.state == "complete" )
				{
					// Remove it from the disk if it exists.
					browser.downloads.removeFile( item.id )
					.then ( function()
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
			} );
		}
	}
}

function GetPOSTRequestInfo( url, type )
{
	var request = null;

	for ( var i = 0; i < g_last_requests.length; ++i )
	{
		if ( g_last_requests[ i ][ 1 ] == url )
		{
			request = g_last_requests[ i ][ type ];
		}
	}

	return request;
}

function GetURLRequest( request )
{
	if ( request.method == "POST" )
	{
		if ( g_last_requests.length > 0 && g_last_requests[ 0 ][ 0 ] == request.requestId )
		{
			g_last_requests[ 0 ][ 2 ] = request.requestBody;
		}
		else
		{
			g_last_requests.unshift( [ request.requestId, request.url, request.requestBody, "" ] );

			if ( g_last_requests.length > 10 )
			{
				g_last_requests.pop();
			}
		}
	}
}

function GetURLHeaders( request )
{
	if ( request.method == "POST" )
	{
		if ( g_last_requests.length > 0 && g_last_requests[ 0 ][ 0 ] == request.requestId )
		{
			for ( let header of request.requestHeaders )
			{
				if ( header.name.toLowerCase() === "content-type" )
				{
					g_last_requests[ 0 ][ 3 ] = header.value;

					break;
				}
			}
		}
		else
		{
			for ( let header of request.requestHeaders )
			{
				if ( header.name.toLowerCase() === "content-type" )
				{
					g_last_requests.unshift( [ request.requestId, request.url, "", header.value ] );

					break;
				}
			}

			if ( g_last_requests.length > 10 )
			{
				g_last_requests.pop();
			}
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

		browser.tabs.executeScript( { file: script_file } )
		.then( function( urls )
		{
			if ( typeof urls == "undefined" )
			{
				urls = "";
			}

			var new_urls = "";
			var ctrl_down = false;

			if ( info.modifiers.length > 0 )
			{
				for ( var i = 0; i < info.modifiers.length; ++i )
				{
					if ( info.modifiers[ i ] == "Ctrl" )
					{
						ctrl_down = true;

						break;
					}
				}
			}

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
							if ( ctrl_down )
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
				browser.cookies.getAllCookieStores()
				.then( function( cookie_stores )
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

		browser.cookies.getAllCookieStores()
		.then( function( cookie_stores )
		{
			GetCookies( { url: url, cookie_stores: cookie_stores, index: 0, domain: "" },
						{ show_add_window: true, id: null, method: "1", url: url, cookie_string: "", headers: headers, directory: "", filename: "", post_data: "" } );
		} );
	}
}

function HandleCommand( command )
{
	if ( command === "override_download_manager" )
	{
		g_options.override = !g_options.override;

		browser.storage.local.set(
		{
			override: g_options.override
		} )
		.then( function()
		{
			HandleMessages( { type: "refresh_options" }, null, ()=>{} );
		} );
	}
	else if ( command === "add_urls_window" )
	{
		CreateDownloadWindow( { show_add_window: true, id: null, method: "1", url: "", cookie_string: "", headers: "", directory: "", filename: "", post_data: "" } );
	}
}

browser.storage.local.get().then( OnGetOptions )
.then( function( options )
{
	g_options = options;

	browser.contextMenus.create(
	{
		id: "download-link",
		title: browser.i18n.getMessage( "menu_download_link" ),
		contexts: [ "link" ]
	} );

	browser.contextMenus.create(
	{
		id: "download-image",
		title: browser.i18n.getMessage( "menu_download_image" ),
		contexts: [ "image" ]
	} );

	browser.contextMenus.create(
	{
		id: "download-audio",
		title: browser.i18n.getMessage( "menu_download_audio" ),
		contexts: [ "audio" ]
	} );

	browser.contextMenus.create(
	{
		id: "download-video",
		title: browser.i18n.getMessage( "menu_download_video" ),
		contexts: [ "video" ]
	} );

	browser.contextMenus.create(
	{
		id: "separator-1",
		type: "separator",
		contexts: [ "link", "image", "audio", "video" ]
	} );

	browser.contextMenus.create(
	{
		id: "download-all-images",
		title: browser.i18n.getMessage( "menu_download_all_images" ),
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	browser.contextMenus.create(
	{
		id: "download-all-media",
		title: browser.i18n.getMessage( "menu_download_all_media" ),
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	browser.contextMenus.create(
	{
		id: "download-all-links",
		title: browser.i18n.getMessage( "menu_download_all_links" ),
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	browser.contextMenus.create(
	{
		id: "separator-2",
		type: "separator",
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	browser.contextMenus.create(
	{
		id: "download-page",
		title: browser.i18n.getMessage( "menu_download_page" ),
		contexts: [ "page", "frame", "link", "image", "audio", "video" ]
	} );

	browser.contextMenus.onClicked.addListener( OnMenuClicked );

	browser.runtime.onMessage.addListener( HandleMessages );

	browser.commands.onCommand.addListener( HandleCommand );

	if ( g_options.override )
	{
		browser.browserAction.setIcon( { path: "icons/icon-e-64.png" } );
		browser.webRequest.onBeforeRequest.addListener( GetURLRequest, { urls: [ "<all_urls>" ] }, [ "requestBody" ] );
		browser.webRequest.onBeforeSendHeaders.addListener( GetURLHeaders, { urls: [ "<all_urls>" ] }, [ "requestHeaders" ] );
		browser.downloads.onCreated.addListener( OnDownloadItemCreated );
	}
} );
