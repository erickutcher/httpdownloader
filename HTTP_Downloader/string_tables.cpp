/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2025 Eric Kutcher

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

#include "globals.h"
#include "string_tables.h"
#include "lite_kernel32.h"
#include "utilities.h"

bool g_use_dynamic_locale = false;	// Did we get the strings from a file? We'll need to free the memory when we're done.

STRING_TABLE_DATA g_locale_table[ TOTAL_LOCALE_STRINGS ];

// Ordered by month.
STRING_TABLE_DATA month_string_table[] =
{
	{ L"January", 7 },
	{ L"February", 8 },
	{ L"March", 5 },
	{ L"April", 5 },
	{ L"May", 3 },
	{ L"June", 4 },
	{ L"July", 4 },
	{ L"August", 6 },
	{ L"September", 9 },
	{ L"October", 7 },
	{ L"November", 8 },
	{ L"December", 8 }
};

// Ordered by day.
STRING_TABLE_DATA day_string_table[] =
{
	{ L"Sunday", 6 },
	{ L"Monday", 6 },
	{ L"Tuesday", 7 },
	{ L"Wednesday", 9 },
	{ L"Thursday", 8 },
	{ L"Friday", 6 },
	{ L"Saturday", 8 }
};

STRING_TABLE_DATA download_string_table[] =
{
	{ L"#", 1 },
	{ L"Active Parts", 12 },
	{ L"Category", 8 },
	{ L"Comments", 8 },
	{ L"Date and Time Added", 19 },
	{ L"Download Directory", 18 },
	{ L"Download Speed", 14 },
	{ L"Download Speed Limit", 20 },
	{ L"Downloaded", 10 },
	{ L"File Size", 9 },
	{ L"File Type", 9 },
	{ L"Filename", 8 },
	{ L"Progress", 8 },
	{ L"SSL / TLS Version", 17 },
	{ L"Time Elapsed", 12 },
	{ L"Time Remaining", 14 },
	{ L"URL", 3 }
};

STRING_TABLE_DATA menu_string_table[] =
{
	{ L"#", 1 },
	{ L"&About", 6 },
	{ L"Active Parts", 12 },
	{ L"Add Category...", 15 },
	{ L"&Add URL(s)...\tCtrl+N", 21 },
	{ L"Add URL(s)...", 13 },
	{ L"Always on Top", 13 },
	{ L"C&ategories", 11 },
	{ L"Category", 8 },
	{ L"&Check for Updates...", 21 },
	{ L"&Column Headers", 15 },
	{ L"Comments", 8 },
	{ L"&Copy URL(s)\tCtrl+C", 19 },
	{ L"Copy URL(s)", 11 },
	{ L"Date and Time Added", 19 },
	{ L"&Delete\tDel", 11 },
	{ L"Delete", 6 },
	{ L"Download Directory", 18 },
	{ L"Download Speed", 14 },
	{ L"Download Speed Limit", 20 },
	{ L"Downloaded", 10 },
	{ L"&Edit", 5 },
	{ L"Enable List &Edit Mode\tCtrl+Shift+E", 35 },
	{ L"Enable List Edit Mode", 21 },
	{ L"E&xit", 5 },
	{ L"Exit", 4 },
	{ L"&Export Download History...", 27 },
	{ L"&File", 5 },
	{ L"File Size", 9 },
	{ L"File Type", 9 },
	{ L"Filename", 8 },
	{ L"Global Download Speed &Limit...\tCtrl+L", 38 },
	{ L"&Help", 5 },
	{ L"HTTP Downloader &Home Page", 26 },
	{ L"&Import Download History...", 27 },
	{ L"Move Down", 9 },
	{ L"Move to Bottom", 14 },
	{ L"Move to Top", 11 },
	{ L"Move Up", 7 },
	{ L"Open Directory", 14 },
	{ L"Open File", 9 },
	{ L"Open Download List", 18 },
	{ L"&Options...\tCtrl+O", 18 },
	{ L"Options...", 10 },
	{ L"&Pause", 6 },
	{ L"Pause", 5 },
	{ L"Pause Active", 12 },
	{ L"Progress", 8 },
	{ L"Queue", 5 },
	{ L"&Remove\tCtrl+R", 14 },
	{ L"Remove", 6 },
	{ L"Remove and Delete\tCtrl+Del", 26 },
	{ L"Remove and Delete", 17 },
	{ L"Remove Completed\tCtrl+Shift+R", 29 },
	{ L"Rename\tF2", 9 },
	{ L"Rename", 6 },
	{ L"Restart", 7 },
	{ L"Resu&me", 7 },
	{ L"Resume", 6 },
	{ L"&Save Download History...", 25 },
	{ L"&Search...\tCtrl+S", 17 },
	{ L"&Select All\tCtrl+A", 18 },
	{ L"Select All", 10 },
	{ L"Site &Manager...\tCtrl+M", 23 },
	{ L"SSL / TLS Version", 17 },
	{ L"St&art", 6 },
	{ L"Start", 5 },
	{ L"Start / Resume Inactive", 23 },
	{ L"&Status Bar", 11 },
	{ L"St&op", 5 },
	{ L"Stop", 4 },
	{ L"Stop All", 8 },
	{ L"Time Elapsed", 12 },
	{ L"Time Remaining", 14 },
	{ L"&Toolbar", 8 },
	{ L"&Tools", 6 },
	{ L"Update Category...", 18 },
	{ L"&Update Download...\tCtrl+U", 26 },
	{ L"Update Download...", 18 },
	{ L"URL", 3 },
	{ L"&View", 5 }
};

STRING_TABLE_DATA options_string_table[] =
{
	{ L"Advanced", 8 },
	{ L"Appearance", 10 },
	{ L"Apply", 5 },
	{ L"Connection", 10 },
	{ L"Fingerprints", 12 },
	{ L"FTP", 3 },
	{ L"General", 7 },
	{ L"OK", 2 },
	{ L"Private Keys", 12 },
	{ L"Proxy", 5 },
	{ L"SFTP", 4 }
};

STRING_TABLE_DATA options_advanced_string_table[] =
{
	{ L"Add in Stopped state", 20 },
	{ L"Allow only one instance of the program to run", 45 },
	{ L"Apply initially set proxy to redirected URL(s)", 46 },
	{ L"Continue Download", 17 },
	{ L"Default download directory:", 27 },
	{ L"Display Prompt", 14 },
	{ L"Download immediately", 20 },
	{ L"Download non-200 and non-206 responses", 38 },
	{ L"Drag and drop URL(s) action:", 28 },
	{ L"Enable download history", 23 },
	{ L"Enable quick file allocation (administrator access required)", 60 },
	{ L"Enable sparse file allocation", 29 },
	{ L"Exit program", 12 },
	{ L"Hibernate", 9 },
	{ L"Hybrid shut down", 16 },
	{ L"Lock", 4 },
	{ L"Log off", 7 },
	{ L"Move deleted downloads to Recycle Bin", 37 },
	{ L"Move files that match category file extension filters", 53 },
	{ L"None", 4 },
	{ L"Override download list action prompts", 37 },
	{ L"Overwrite File", 14 },
	{ L"Prevent system standby while downloads are active", 49 },
	{ L"Rename File", 11 },
	{ L"Restart", 7 },
	{ L"Restart Download", 16 },
	{ L"Resume previously downloading files upon startup", 48 },
	{ L"Set date and time of file from server response", 46 },
	{ L"Shut down", 9 },
	{ L"Skip Download", 13 },
	{ L"Sleep", 5 },
	{ L"System shutdown action when all downloads finish:", 49 },
	{ L"Thread pool count:", 18 },
	{ L"Update redirected URL(s) in download list", 41 },
	{ L"Use temporary download directory:", 33 },
	{ L"When a file already exists:", 27 },
	{ L"When a file has been modified:", 30 },
	{ L"When a file is greater than or equal to (bytes):", 48 }
};

STRING_TABLE_DATA options_appearance_string_table[] =
{
	{ L"Background Color", 16 },
	{ L"Background Font Color", 21 },
	{ L"Border Color", 12 },
	{ L"Download list:", 14 },
	{ L"Draw all rows", 13 },
	{ L"Draw full rows", 14 },
	{ L"Even Row Background Color", 25 },
	{ L"Even Row Font", 13 },
	{ L"Even Row Font Color", 19 },
	{ L"Even Row Highlight Color", 24 },
	{ L"Even Row Highlight Font Color", 29 },
	{ L"Expand added group items", 24 },
	{ L"Gridline Color", 14 },
	{ L"Odd Row Background Color", 24 },
	{ L"Odd Row Font", 12 },
	{ L"Odd Row Font Color", 18 },
	{ L"Odd Row Highlight Color", 23 },
	{ L"Odd Row Highlight Font Color", 28 },
	{ L"Other progress bars:", 20 },
	{ L"Progress Color", 14 },
	{ L"Progress bar:", 13 },
	{ L"Progress Font Color", 19 },
	{ L"Scroll to last item when adding URL(s)", 38 },
	{ L"Selection Marquee Color", 23 },
	{ L"Show executable's embedded icon", 31 },
	{ L"Show gridlines in download list", 31 },
	{ L"Show progress for each part", 27 },
	{ L"Sort added and updating items", 29 },
	{ L"System Tray Icon Downloading", 28 },
	{ L"System Tray Icon Paused", 23 },
	{ L"System Tray Icon Error", 22 },
	{ L"URL Drop Window Downloading", 27 },
	{ L"URL Drop Window Paused", 22 },
	{ L"URL Drop Window Error", 21 }
};

STRING_TABLE_DATA options_connection_string_table[] =
{
	{ L"Active download limit:", 22 },
	{ L"Default download parts:", 23 },
	{ L"Default SSL / TLS version:", 26 },
	{ L"Maximum redirects:", 18 },
	{ L"Reallocate parts to maximize connections:", 41 },
	{ L"Reallocate threshold size (bytes):", 34 },
	{ L"Retry incomplete downloads:", 27 },
	{ L"Retry incomplete parts:", 23 },
	{ L"Timeout (seconds):", 18 }
};

STRING_TABLE_DATA options_ftp_string_table[] =
{
	{ L"-", 1 },
	{ L"Active", 6 },
	{ L"Active Listen Information", 25 },
	{ L"Data Transfer Mode", 18 },
	{ L"Passive", 7 },
	{ L"Port end:", 9 },
	{ L"Port start:", 11 },
	{ L"Send keep-alive requests", 24 },
	{ L"Use other mode on failure", 25 }
};

STRING_TABLE_DATA options_general_string_table[] =
{
	{ L"Always on top", 13 },
	{ L"Check for updates upon startup", 30 },
	{ L"Close to System Tray", 20 },
	{ L"Enable System Tray icon:", 24 },
	{ L"Enable URL drop window:", 23 },
	{ L"Failure:", 8 },
	{ L"Load Download Finish Sound File", 31 },
	{ L"Minimize to System Tray", 23 },
	{ L"Play sound when all downloads finish", 36 },
	{ L"Show notification when all downloads finish", 43 },
	{ L"Show progress bar", 17 },
	{ L"Start in System Tray", 20 },
	{ L"Success:", 8 },
	{ L"Transparency:", 13 },
	{ L"\x25B6", 2 }
};

STRING_TABLE_DATA options_proxy_string_table[] =
{
	{ L"Allow proxy to resolve domain names", 35 },
	{ L"Allow proxy to resolve domain names (use SOCKS v4a)", 51 },
	{ L"Hostname:", 9 },
	{ L"SOCKS v4", 8 },
	{ L"SOCKS v5", 8 },
	{ L"Use authentication:", 19 },
	{ L"Use HTTP proxy:", 15 },
	{ L"Use HTTPS proxy:", 16 },
	{ L"Use SOCKS proxy:", 16 }
};

STRING_TABLE_DATA options_server_string_table[] =
{
	{ L":", 1 },
	{ L"Basic Authentication", 20 },
	{ L"Certificate file:", 17 },
	{ L"Digest Authentication", 21 },
	{ L"Enable server:", 14 },
	{ L"Enable SSL / TLS:", 17 },
	{ L"Hostname / IPv6 address:", 24 },
	{ L"IPv4 address:", 13 },
	{ L"Key file:", 9 },
	{ L"Load PKCS #12 File", 18 },
	{ L"Load Private Key File", 21 },
	{ L"Load X.509 Certificate File", 27 },
	{ L"PKCS #12:", 9 },
	{ L"PKCS #12 file:", 14 },
	{ L"PKCS #12 password:", 18 },
	{ L"Port:", 5 },
	{ L"Public / Private key pair:", 26 },
	{ L"Require authentication:", 23 },
	{ L"Server", 6 },
	{ L"Server SSL / TLS version:", 25 },
	{ L"Show System Tray notification for remote connections", 52 }
};

STRING_TABLE_DATA options_sftp_string_table[] =
{
	{ L"Algorithm Selection Policies", 28 },
	{ L"Attempt GSSAPI authentication", 29 },
	{ L"Attempt GSSAPI key exchange", 27 },
	{ L"Drag items to reorder priority.", 31 },
	{ L"Enable compression", 18 },
	{ L"Encryption cipher:", 18 },
	{ L"Fingerprint", 11 },
	{ L"Fingerprint:", 12 },
	{ L"Fingerprints:", 13 },
	{ L"GSS rekey time (minutes):", 25 },
	{ L"Host", 4 },
	{ L"Host:", 5 },
	{ L"Host key:", 9 },
	{ L"Host Key Algorithm", 18 },
	{ L"Host key algorithm:", 19 },
	{ L"Key/Group exchange:", 19 },
	{ L"Load Private Key", 16 },
	{ L"Private Key File", 16 },
	{ L"Private key file:", 17 },
	{ L"Rekey data limit (bytes):", 25 },
	{ L"Rekey time (minutes):", 21 },
	{ L"Send keep-alive requests (seconds):", 35 }
};

STRING_TABLE_DATA cmessagebox_string_table[] =
{
	{ L"Continue", 8 },
	{ L"No", 2 },
	{ L"Overwrite", 9 },
	{ L"Remember choice", 15 },
	{ L"Skip", 4 },
	{ L"Skip remaining messages", 23 },
	{ L"Yes", 3 }
};

STRING_TABLE_DATA add_urls_string_table[] =
{
	{ L"Advanced options", 16 },
	{ L"Authentication", 14 },
	{ L"Cookies", 7 },
	{ L"Cookies:", 8 },
	{ L"Custom", 6 },
	{ L"Download", 8 },
	{ L"Download directory:", 19 },
	{ L"Download parts:", 15 },
	{ L"Headers", 7 },
	{ L"Headers:", 8 },
	{ L"Images", 6 },
	{ L"Music", 5 },
	{ L"Password:", 9 },
	{ L"POST Data", 9 },
	{ L"RegEx filter:", 13 },
	{ L"Send POST Data:", 15 },
	{ L"Simulate download", 17 },
	{ L"SSL / TLS version:", 18 },
	{ L"URL(s):", 7 },
	{ L"Use proxy:", 10 },
	{ L"Username:", 9 },
	{ L"Verify", 6 },
	{ L"Videos", 6 }
};

STRING_TABLE_DATA search_string_table[] =
{
	{ L"Match case", 10 },
	{ L"Match whole word", 16 },
	{ L"Regular expression", 18 },
	{ L"Search", 6 },
	{ L"Search All", 10 },
	{ L"Search for:", 11 },
	{ L"Search Next", 11 },
	{ L"Search Type", 11 }
};

STRING_TABLE_DATA site_manager_string_table[] =
{
	{ L"********", 8 },
	{ L"[...]", 5 },
	{ L"Add", 3 },
	{ L"Close", 5 },
	{ L"Disable", 7 },
	{ L"Download operation:", 19 },
	{ L"Download Operations", 19 },
	{ L"Download Parts", 14 },
	{ L"Empty Body", 10 },
	{ L"Enable", 6 },
	{ L"New", 3 },
	{ L"Password", 8 },
	{ L"Proxy Password", 14 },
	{ L"Proxy Port", 10 },
	{ L"Proxy Server", 12 },
	{ L"Proxy Type", 10 },
	{ L"Proxy Username", 14 },
	{ L"Resolve Domain Names", 20 },
	{ L"Save", 4 },
	{ L"Show passwords", 14 },
	{ L"Simulate", 8 },
	{ L"Site", 4 },
	{ L"Site:", 5 },
	{ L"Username", 8 }
};

STRING_TABLE_DATA add_category_string_table[] =
{
	{ L"Associate file extension(s) with category:", 42 }
};

STRING_TABLE_DATA fingerprint_prompt_string_table[] =
{
	{ L"Add host and key information to cache", 37 },
	{ L"Key size:", 9 }
};

STRING_TABLE_DATA update_check_string_table[] =
{
	{ L"A new version is available.", 27 },
	{ L"Checking for updates...", 23 },
	{ L"Current version:", 16 },
	{ L"Download Update", 15 },
	{ L"HTTP Downloader is up to date.", 30 },
	{ L"Latest version:", 15 },
	{ L"The update check has failed.", 28 },
	{ L"View Changelog", 14 },
	{ L"Visit Home Page", 15 }
};

STRING_TABLE_DATA common_string_table[] =
{
	{ L"...", 3 },
	{ L"[Simulated]", 11 },
	{ L"Add Category", 12 },
	{ L"Add URL(s)", 10 },
	{ L"Added", 5 },
	{ L"All", 3 },
	{ L"Allocating File", 15 },
	{ L"Authorization Required", 22 },
	{ L"Cancel", 6 },
	{ L"Categories", 10 },
	{ L"Category:", 9 },
	{ L"Check For Updates", 17 },
	{ L"Comments:", 9 },
	{ L"Completed", 9 },
	{ L"Connecting", 10 },
	{ L"Default", 7 },
	{ L"Default download speed limit (bytes/s):", 39 },
	{ L"Desktop", 7 },
	{ L"Documents", 9 },
	{ L"Download speed:", 15 },
	{ L"Download speed limit (bytes/s):", 31 },
	{ L"Downloading", 11 },
	{ L"Downloads", 9 },
	{ L"Downloads Have Finished", 23 },
	{ L"Error", 5 },
	{ L"Export Download History", 23 },
	{ L"Failed", 6 },
	{ L"File IO Error", 13 },
	{ L"Global Download Speed Limit", 27 },
	{ L"Global download speed limit:", 28 },
	{ L"Global download speed limit (bytes/s):", 38 },
	{ L"HTTP", 4 },
	{ L"HTTPS", 5 },
	{ L"Import Download History", 23 },
	{ L"Insufficient Disk Space", 23 },
	{ L"IP address:", 11 },
	{ L"Items:", 6 },
	{ L"Moving File", 11 },
	{ L"Music", 5 },
	{ L"Options", 7 },
	{ L"Other", 5 },
	{ L"Paused", 6 },
	{ L"Pictures", 8 },
	{ L"Proxy Authentication Required", 29 },
	{ L"Queued", 6 },
	{ L"Remote Connection", 17 },
	{ L"Restarting", 10 },
	{ L"Save Download History", 21 },
	{ L"Set", 3 },
	{ L"Site Manager", 12 },
	{ L"Skipped", 7 },
	{ L"Status", 6 },
	{ L"Stopped", 7 },
	{ L"SSL 2.0", 7 },
	{ L"SSL 3.0", 7 },
	{ L"Timed Out", 9 },
	{ L"TLS 1.0", 7 },
	{ L"TLS 1.1", 7 },
	{ L"TLS 1.2", 7 },
	{ L"TLS 1.3", 7 },
	{ L"Total downloaded:", 17 },
	{ L"Unlimited", 9 },
	{ L"Update", 6 },
	{ L"Update Category", 15 },
	{ L"Update Download", 15 },
	{ L"URL:", 4 },
	{ L"URL(s) added:", 13 },
	{ L"Videos", 6 }
};

STRING_TABLE_DATA common_message_string_table[] =
{
	{ L"A key algorithm must be supplied.", 33 },
	{ L"A private key file is required.", 31 },
	{ L"A protocol (HTTP(S), FTP(S), or SFTP) must be supplied.", 55 },
	{ L"A restart is required for these changes to take effect.", 55 },
	{ L"A restart is required to disable quick file allocation.", 55 },
	{ L"A restart is required to enable quick file allocation.", 54 },
	{ L"A restart is required to perform the system shutdown action.", 60 },
	{ L"A restart is required to update the thread pool count.", 54 },
	{ L"Are you sure you want to delete the selected files?", 51 },
	{ L"Are you sure you want to remove the completed entries?", 54 },
	{ L"Are you sure you want to remove and delete the selected entries?", 64 },
	{ L"Are you sure you want to remove the selected entries?", 53 },
	{ L"Are you sure you want to restart the selected entries?", 54 },
	{ L"Do you want to accept the server's host key?", 44 },
	{ L"Item ID List was not freed.", 27 },
	{ L"One or more file extensions already exist in another category.", 62 },
	{ L"One or more files are in use and cannot be deleted.", 51 },
	{ L"One or more files were not found.", 33 },
	{ L"One or more paths were not found.", 33 },
	{ L"Select the category download directory.", 39 },
	{ L"Select the default download directory.", 38 },
	{ L"Select the download directory.", 30 },
	{ L"Select the temporary download directory.", 40 },
	{ L"The download will be resumed after it's updated.", 48 },
	{ L"The file is currently in use and cannot be deleted.", 51 },
	{ L"The file is currently in use and cannot be renamed.", 51 },
	{ L"The file(s) could not be imported because the format is incorrect.", 66 },
	{ L"The key fingerprint does not match the cached entry.\r\nDo you want to accept the server's host key?", 98 },
	{ L"The specified category already exists.", 38 },
	{ L"The specified file was not found.", 33 },
	{ L"The specified file was not found.\r\n\r\nDo you want to download the file again?", 76 },
	{ L"The specified host already exists.", 34 },
	{ L"The specified host is invalid.", 30 },
	{ L"The specified path was not found.", 33 },
	{ L"The specified site already exists.", 34 },
	{ L"The specified site is invalid.", 30 },
	{ L"The specified username and host already exists.", 47 },
	{ L"There is already a file with the same name in this location.", 60 },
	{ L"You must supply a download directory.", 37 }
};

STRING_TABLE_DATA about_string_table[] =
{
	{ L"Beta", 4 },
	{ L"Built on", 8 },
	{ L"Copyright", 9 },
	{ L"HTTP Downloader is made free under the GPLv3 license.", 53 },
	{ L"Version", 7 }
};

STRING_TABLE_DATA dynamic_message_string_table[] =
{
	{ L"%s already exists.\r\n\r\nWhat operation would you like to perform?", 63 },
	{ L"%s could not be renamed.\r\n\r\nYou will need to choose a different save directory.", 79 },
	{ L"%s has been modified.\r\n\r\nWhat operation would you like to perform?", 66 },
	{ L"%s will be %I64u bytes in size.\r\n\r\nDo you want to continue downloading this file?", 81 }
};

// Ordered by the enum values in putty.h
STRING_TABLE_DATA sftp_kex_string_table[] =
{
	{ L"DH-Group1-SHA1", 14 },
	{ L"DH-Group14-SHA1", 15 },
	{ L"DH-GEX-SHA1", 11 },
	{ L"RSA", 3 },
	{ L"ECDH", 4 }
};

// Ordered by the enum values in putty.h
STRING_TABLE_DATA sftp_hk_string_table[] =
{
	{ L"RSA", 3 },
	{ L"DSA", 3 },
	{ L"ECDSA", 5 },
	{ L"Ed25519", 7 }
};

// Ordered by the enum values in putty.h
STRING_TABLE_DATA sftp_ec_string_table[] =
{
	{ L"3DES", 4 },
	{ L"Blowfish", 8 },
	{ L"AES", 3 },
	{ L"DES", 3 },
	{ L"Arcfour", 7 },
	{ L"ChaCha20", 8 }
};

// Host key algorithms.
STRING_TABLE_DATA sftp_hka_string_table[] =
{
	{ L"ecdsa-sha2-nistp256", 19 },
	{ L"ecdsa-sha2-nistp384", 19 },
	{ L"ecdsa-sha2-nistp521", 19 },
	{ L"ssh-dss", 7 },
	{ L"ssh-ed25519", 11 },
	{ L"ssh-rsa", 7 }
};

// File filters.
STRING_TABLE_DATA file_filters_string_table[] =
{
	{ L"All Files", 9 },
	{ L"CSV (Comma delimited)", 21 },
	{ L"Download History", 16 },
	{ L"Personal Information Exchange", 29 },
	{ L"Private Keys", 12 },
	{ L"PuTTY Private Key Files", 23 },
	{ L"WAV", 3 },
	{ L"X.509 Certificates", 18 }
};

void InitializeLocaleValues()
{
	unsigned short string_count = 0;

	bool use_locale_file = true;

	char open_count = 0;

	//_memzero( g_locale_table, sizeof( STRING_TABLE_DATA ) * TOTAL_LOCALE_STRINGS );

	unsigned char i;

	for ( i = 0; i < MONTH_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = month_string_table[ i ]; }
	for ( i = 0; i < DAY_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = day_string_table[ i ]; }
	for ( i = 0; i < DOWNLOAD_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = download_string_table[ i ]; }
	for ( i = 0; i < MENU_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = menu_string_table[ i ]; }
	for ( i = 0; i < OPTIONS_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_string_table[ i ]; }
	for ( i = 0; i < OPTIONS_ADVANCED_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_advanced_string_table[ i ]; }
	for ( i = 0; i < OPTIONS_APPEARANCE_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_appearance_string_table[ i ]; }
	for ( i = 0; i < OPTIONS_CONNECTION_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_connection_string_table[ i ]; }
	for ( i = 0; i < OPTIONS_FTP_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_ftp_string_table[ i ]; }
	for ( i = 0; i < OPTIONS_GENERAL_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_general_string_table[ i ]; }
	for ( i = 0; i < OPTIONS_PROXY_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_proxy_string_table[ i ]; }
	for ( i = 0; i < OPTIONS_SERVER_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_server_string_table[ i ]; }
	for ( i = 0; i < OPTIONS_SFTP_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_sftp_string_table[ i ]; }
	for ( i = 0; i < CMESSAGEBOX_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = cmessagebox_string_table[ i ]; }
	for ( i = 0; i < ADD_URLS_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = add_urls_string_table[ i ]; }
	for ( i = 0; i < SEARCH_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = search_string_table[ i ]; }
	for ( i = 0; i < SITE_MANAGER_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = site_manager_string_table[ i ]; }
	for ( i = 0; i < ADD_CATEGORY_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = add_category_string_table[ i ]; }
	for ( i = 0; i < FINGERPRINT_PROMPT_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = fingerprint_prompt_string_table[ i ]; }
	for ( i = 0; i < UPDATE_CHECK_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = update_check_string_table[ i ]; }
	for ( i = 0; i < COMMON_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = common_string_table[ i ]; }
	for ( i = 0; i < COMMON_MESSAGE_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = common_message_string_table[ i ]; }
	for ( i = 0; i < ABOUT_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = about_string_table[ i ]; }
	for ( i = 0; i < DYNAMIC_MESSAGE_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = dynamic_message_string_table[ i ]; }
	for ( i = 0; i < SFTP_KEX_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = sftp_kex_string_table[ i ]; }
	for ( i = 0; i < SFTP_HK_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = sftp_hk_string_table[ i ]; }
	for ( i = 0; i < SFTP_EC_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = sftp_ec_string_table[ i ]; }
	for ( i = 0; i < SFTP_HKA_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = sftp_hka_string_table[ i ]; }
	for ( i = 0; i < FILE_FILTERS_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = file_filters_string_table[ i ]; }

	string_count = 0;

	/*
	// Quick locale generation.
	HANDLE hFile_locale = CreateFile( L"en-US", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_locale != INVALID_HANDLE_VALUE )
	{
		DWORD write = 0;

		for ( short j = 0; j < TOTAL_LOCALE_STRINGS; ++j )
		{
			// Include NULL terminator.
			WriteFile( hFile_locale, g_locale_table[ j ].value, sizeof( wchar_t ) * ( g_locale_table[ j ].length + 1 ), &write, NULL );
		}

		CloseHandle( hFile_locale );
	}
	*/
	/*
	// Quick string list generation.
	HANDLE hFile_string_list = CreateFile( L"string_list.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_string_list != INVALID_HANDLE_VALUE )
	{
		DWORD write = 0;

		for ( short j = 0; j < TOTAL_LOCALE_STRINGS; ++j )
		{
			WriteFile( hFile_string_list, g_locale_table[ j ].value, sizeof( wchar_t ) * g_locale_table[ j ].length, &write, NULL );
			WriteFile( hFile_string_list, L"\r\n", sizeof( wchar_t ) * 2, &write, NULL );
		}

		CloseHandle( hFile_string_list );
	}
	*/

	//wchar_t directory[ MAX_PATH ];
	//int directory_length = GetCurrentDirectoryW( MAX_PATH, directory );
	//int directory_length = GetModuleFileNameW( NULL, directory, MAX_PATH );
	//while ( directory_length != 0 && directory[ --directory_length ] != L'\\' );
	//directory[ directory_length ] = 0;	// Sanity.

	// Find the default locale.
	_wmemcpy_s( g_program_directory + g_program_directory_length, MAX_PATH - g_program_directory_length, L"\\locale\\default\0", 16 );
	//g_program_directory[ g_program_directory_length + 15 ] = 0;	// Sanity.

	if ( GetFileAttributesW( g_program_directory ) == INVALID_FILE_ATTRIBUTES )
	{
		int locale_length = 0;

		// Make sure GetUserDefaultLocaleName is available on our system.
		if ( kernel32_state != KERNEL32_STATE_SHUTDOWN )
		{
			// LOCALE_NAME_MAX_LENGTH
			// Find a specific locale based on the system's default.
			locale_length = _GetUserDefaultLocaleName( g_program_directory + g_program_directory_length + 8, MAX_PATH - ( g_program_directory_length + 8 ) );
			g_program_directory[ g_program_directory_length + 8 + locale_length - 1 ] = 0;	// Sanity.
		}

		if ( locale_length == 0 )
		{
			use_locale_file = false;
		}
	}

	if ( use_locale_file )
	{
		HANDLE hFile_locale = INVALID_HANDLE_VALUE;

RETRY_OPEN:

		hFile_locale = CreateFile( g_program_directory, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile_locale != INVALID_HANDLE_VALUE )
		{
			OVERLAPPED lfo;
			_memzero( &lfo, sizeof( OVERLAPPED ) );
			LockFileEx( hFile_locale, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

			unsigned char *locale_buf = NULL;
			DWORD read = 0;
			DWORD fz = GetFileSize( hFile_locale, NULL );

			if ( fz > sizeof( wchar_t ) && fz < 131072 )
			{
				locale_buf = ( unsigned char * )GlobalAlloc( GMEM_FIXED, sizeof( unsigned char ) * fz + 2 );
				if ( locale_buf != NULL )
				{
					// Look for a UTF-16 BOM (little endian or big endian) and ignore it.
					BOOL bRet = ReadFile( hFile_locale, locale_buf, sizeof( unsigned char ) * 2, &read, NULL );
					if ( bRet != FALSE )
					{
						if ( read == 2 && ( ( locale_buf[ 0 ] == 0xFF && locale_buf[ 1 ] == 0xFE ) ||
											( locale_buf[ 0 ] == 0xFE && locale_buf[ 1 ] == 0xFF ) ) )
						{
							read = 0;
							fz -= 2;
						}
						bRet = ReadFile( hFile_locale, locale_buf + read, ( sizeof( unsigned char ) * fz ) - read, &read, NULL );
						if ( bRet != FALSE )
						{
							// Guarantee a NULL terminated (wide character) buffer.
							locale_buf[ fz ] = 0;
							locale_buf[ fz + 1 ] = 0;
						}
						else
						{
							fz = 0;
						}
					}
					else
					{
						fz = 0;
					}
				}
			}
			/*else
			{
				use_locale_file = false;	// Incorrect file size.
			}*/

			UnlockFileEx( hFile_locale, 0, MAXDWORD, MAXDWORD, &lfo );

			CloseHandle( hFile_locale );

			if ( fz > sizeof( wchar_t ) )
			{
				wchar_t *ptr = ( wchar_t * )locale_buf;
				wchar_t *last_ptr = ptr;
				wchar_t *ptr_end = ( wchar_t * )( ( unsigned char * )( locale_buf + fz ) );

				while ( string_count < TOTAL_LOCALE_STRINGS && ++ptr < ptr_end )
				{
					if ( *ptr == NULL )
					{
						g_locale_table[ string_count ].value = last_ptr;
						g_locale_table[ string_count ].length = ( unsigned short )( ptr - last_ptr );

						++ptr;
						last_ptr = ptr;

						++string_count;
					}
				}

				g_use_dynamic_locale = true;

#ifdef ENABLE_LOGGING
				WriteLog( LOG_INFO_MISC, "Using locale file: %S", g_program_directory );
#endif
			}
			else
			{
				GlobalFree( locale_buf );

				//use_locale_file = false;	// Incorrect file size.
			}
		}
		else
		{
			if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
			{
				Sleep( 200 );
				goto RETRY_OPEN;
			}

			//use_locale_file = false;	// Can't open file for reading.
		}
	}
}

void UninitializeLocaleValues()
{
	if ( g_use_dynamic_locale )
	{
		// The first pointer points to the entire buffer that we read from the file.
		// So we only need to delete it and nothing else.
		GlobalFree( g_locale_table[ 0 ].value );
	}
}
