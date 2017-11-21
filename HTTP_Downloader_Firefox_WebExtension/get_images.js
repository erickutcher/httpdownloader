function GetImages()
{
	var urls_string = "";
	var urls = document.body.getElementsByTagName( "img" );
	var num_urls = urls.length;

	for ( var i = 0; i < num_urls; ++i )
	{
		if ( urls[ i ].src != "" )
		{
			urls_string += urls[ i ].src + "\r\n";
		}
	}

	return urls_string;
}

GetImages();
