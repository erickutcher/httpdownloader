The locale files that have been generated here are loaded by HTTP Downloader based on the user's default locale name. Windows XP and Windows 2000 users will need to determine what their locale is, copy the appropriate file, and rename it to "default". The "default" locale will also take precedence over the user's default locale if the locale name can be determined (Windows Vista and newer systems).

Each locale file is a concatenation of the strings found in "string_list.txt" where all of the carriage return and newline character pairs have been replaced with null, and the escaped string values have been replaced with their binary equivalents.

The included "locale_generator.exe" program will generate a compatible file if you'd like to write your own translation. All you need to do is translate the strings in "string_list.txt" and run the following command from the Command Prompt: 

locale_generator.exe --string-list "string_list.txt"

Running the above command will generate a locale based on the user's default locale name. To force the output to use another locale name, append the locale name to the string list file name. For example, rename "string_list.txt" to "string_list_en-US.txt".

locale_generator.exe --string-list "string_list_en-US.txt"

The above command will output a locale with the name "en-US".

Additionally, you can covert a locale file back to a string list by running the following command (replace "en-US" with the appropriate locale file):

locale_generator.exe --locale "en-US"