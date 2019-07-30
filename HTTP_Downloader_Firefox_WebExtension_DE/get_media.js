function GetMedia()
{
	var urls_string = "";
	var tags = [ "audio", "video", "source" ];

	for ( var t = 0; t < 3; ++t )
	{
		var urls = document.body.getElementsByTagName( tags[ t ] );

		for ( var i = 0; i < urls.length; ++i )
		{
			if ( urls[ i ].src != "" )
			{
				urls_string += urls[ i ].src + "\r\n";
			}
		}
	}

	return urls_string;
}

GetMedia();
