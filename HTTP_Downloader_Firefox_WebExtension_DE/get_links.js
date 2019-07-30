function GetLinks()
{
	var urls_string = "";
	var urls = document.body.getElementsByTagName( "a" );

	for ( var i = 0; i < urls.length; ++i )
	{
		if ( urls[ i ].href != "" )
		{
			urls_string += urls[ i ].href + "\r\n";
		}
	}

	return urls_string;
}

GetLinks();
