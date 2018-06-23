/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2018 Eric Kutcher

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "string_tables.h"

// Ordered by month.
wchar_t *month_string_table[] =
{
	L"January",
	L"February",
	L"March",
	L"April",
	L"May",
	L"June",
	L"July",
	L"August",
	L"September",
	L"October",
	L"November",
	L"December"
};

// Ordered by day.
wchar_t *day_string_table[] =
{
	L"Sunday",
	L"Monday",
	L"Tuesday",
	L"Wednesday",
	L"Thursday",
	L"Friday",
	L"Saturday"
};

wchar_t *download_string_table[] =
{
	L"#",
	L"Active Parts",
	L"Date and Time Added",
	L"Download Directory",
	L"Download Speed",
	L"Downloaded",
	L"File Size",
	L"File Type",
	L"Filename",
	L"Progress",
	L"Time Elapsed",
	L"Time Remaining",
	L"TLS / SSL Version",
	L"URL"
};

wchar_t *menu_string_table[] =
{
	L"#",
	L"&About",
	L"&Add URL(s)...",
	L"&Edit",
	L"&File",
	L"&Help",
	L"&Options...",
	L"&Pause",
	L"&Remove Selected",
	L"&Select All",
	L"&Tools",
	L"&View",
	L"Active Parts",
	L"Add URL(s)...",
	L"Copy URL(s)",
	L"Date and Time Added",
	L"Download Directory",
	L"Download Speed",
	L"Downloaded",
	L"E&xit",
	L"Exit",
	L"File Size",
	L"File Type",
	L"Filename",
	L"HTTP Downloader &Home Page",
	L"Move Down",
	L"Move To Bottom",
	L"Move To Top",
	L"Move Up",
	L"Open Directory",
	L"Open File",
	L"Open Download List",
	L"Options...",
	L"Pause",
	L"Pause Active",
	L"Progress",
	L"Queue",
	L"Remove Completed",
	L"Remove Selected",
	L"Select All",
	L"St&art",
	L"Start",
	L"&Status Bar",
	L"St&op",
	L"Stop",
	L"Stop All",
	L"Time Elapsed",
	L"Time Remaining",
	L"TLS / SSL Version",
	L"Update Download...",
	L"URL"
};

wchar_t *options_string_table[] =
{
	L":",
	L"Active download limit:",
	L"Always on top",
	L"Apply",
	L"Basic Authentication",
	L"Certificate file:",
	L"Close to System Tray",
	L"Configure Proxies",
	L"Connection",
	L"Default download directory:",
	L"Default download parts:",
	L"Default SSL / TLS version:",
	L"Digest Authentication",
	L"Enable download history",
	L"Enable HTTP proxy",
	L"Enable HTTPS proxy",
	L"Enable quick file allocation (administrator access required)",
	L"Enable server:",
	L"Enable SSL / TLS:",
	L"Enable System Tray icon:",
	L"General",
	L"Hostname / IPv6 address:",
	L"IPv4 address:",
	L"Key file:",
	L"Load PKCS #12 File",
	L"Load Private Key File",
	L"Load X.509 Certificate File",
	L"Maximum redirects:",
	L"Minimize to System Tray",
	L"OK",
	L"PKCS #12:",
	L"PKCS #12 file:",
	L"PKCS #12 password:",
	L"Port:",
	L"Proxy",
	L"Public / Private key pair:",
	L"Require authentication:",
	L"Retry incomplete downloads:",
	L"Retry incomplete parts:",
	L"Server SSL / TLS version:",
	L"Thread pool count:",
	L"Timeout (seconds):"
};

wchar_t *add_urls_string_table[] =
{
	L"Advanced \xAB",
	L"Advanced \xBB",
	L"Authentication",
	L"Cookies:",
	L"Download",
	L"Download directory:",
	L"Download parts:",
	L"Headers:",
	L"Password:",
	L"Simulate download",
	L"SSL / TLS version:",
	L"URL(s):",
	L"Username:"
};

wchar_t *common_string_table[] =
{
	L"...",
	L"[Simulated]",
	L"Add URL(s)",
	L"Allocating File",
	L"Authorization Required",
	L"Cancel",
	L"Completed",
	L"Connecting",
	L"Download speed: ",
	L"Download speed: 0 B/s",
	L"Download speed: 0.00 GB/s",
	L"Download speed: 0.00 KB/s",
	L"Download speed: 0.00 MB/s",
	L"Failed",
	L"File IO Error",
	L"Options",
	L"Paused",
	L"Proxy Authentication Required",
	L"Queued",
	L"Skipped",
	L"Stopped",
	L"SSL 2.0",
	L"SSL 3.0",
	L"Timed Out",
	L"TLS 1.0",
	L"TLS 1.1",
	L"TLS 1.2",
	L"Total downloaded: ",
	L"Total downloaded: 0 B",
	L"Total downloaded: 0.00 GB",
	L"Total downloaded: 0.00 KB",
	L"Total downloaded: 0.00 MB",
	L"URL:",
	L"Update",
	L"Update Download"
};

wchar_t *common_message_string_table[] =
{
	L"A restart is required for these changes to take effect.",
	L"A restart is required to enable quick file allocation.",
	L"A restart is required to update the thread pool count.",
	L"Select the default download directory.",
	L"Select the download directory.",
	L"The download will be resumed after it's updated.",
	L"The specified file was not found.\r\n\r\nDo you want to download the file again?",
	L"The specified path was not found.",
	L"You must supply a download directory."
};
