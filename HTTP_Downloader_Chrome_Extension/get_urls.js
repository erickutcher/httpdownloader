function GetImages()
{
	var url_array = [];
	var urls = document.body.getElementsByTagName( "img" );

	for ( var i = 0; i < urls.length; ++i )
	{
		if ( urls[ i ].src != "" )
		{
			var protocol = urls[ i ].src.substring( 0, 8 ).toLowerCase();

			if ( protocol.startsWith( "https:" ) ||
				 protocol.startsWith( "http:" ) ||
				 protocol.startsWith( "ftp:" ) ||
				 protocol.startsWith( "sftp:" ) )
			{
				url_array.push( urls[ i ].src );
			}
		}
	}

	var url_set = [ ...new Set( url_array ) ];

	return url_set.join( "\r\n" ) + "\r\n";
}

function GetMedia()
{
	var url_array = [];
	var tags = [ "audio", "video", "source" ];

	for ( var t = 0; t < 3; ++t )
	{
		var urls = document.body.getElementsByTagName( tags[ t ] );

		for ( var i = 0; i < urls.length; ++i )
		{
			if ( urls[ i ].src != "" )
			{
				var protocol = urls[ i ].src.substring( 0, 8 ).toLowerCase();

				if ( protocol.startsWith( "https:" ) ||
					 protocol.startsWith( "http:" ) ||
					 protocol.startsWith( "ftp:" ) ||
					 protocol.startsWith( "sftp:" ) )
				{
					url_array.push( urls[ i ].src );
				}
			}
		}
	}

	var url_set = [ ...new Set( url_array ) ];

	return url_set.join( "\r\n" ) + "\r\n";
}

function GetLinks()
{
	var url_array = [];
	var urls = document.body.getElementsByTagName( "a" );

	for ( var i = 0; i < urls.length; ++i )
	{
		if ( urls[ i ].href != "" )
		{
			var protocol = urls[ i ].href.substring( 0, 8 ).toLowerCase();

			if ( protocol.startsWith( "https:" ) ||
				 protocol.startsWith( "http:" ) ||
				 protocol.startsWith( "ftp:" ) ||
				 protocol.startsWith( "sftp:" ) )
			{
				url_array.push( urls[ i ].href );
			}
		}
	}
	
	var url_set = [ ...new Set( url_array ) ];

	return url_set.join( "\r\n" ) + "\r\n";
}

switch ( self.downloadType )
{
	case 1: { GetImages(); } break;
	case 2: { GetMedia(); } break;
	case 3: { GetLinks(); } break;
}
