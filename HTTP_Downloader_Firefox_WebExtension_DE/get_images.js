function GetImages()
{
	var urls_string = "";
	var urls = document.body.getElementsByTagName( "img" );

	for ( var i = 0; i < urls.length; ++i )
	{
		if ( urls[ i ].src != "" )
		{
			urls_string += urls[ i ].src + "\r\n";
		}
	}

	return urls_string;
}

GetImages();
