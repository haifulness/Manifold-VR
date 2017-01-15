//	GeometryGamesLocalization.c
//
//	Manages a dictionary for translating all 
//	user-visible phrases to the current (human) language.
//
//	If necessary the dictionary could be implemented 
//	as a binary tree, but in practice the dictionary 
//	will be small (~100 entries) and lookups will occur 
//	only rarely (for example, when changing the active language), 
//	so a simple linear list should be fine.  
//	More precisely, we'll use a NULL-terminated singly-linked list.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGamesLocalization.h"
#include <string.h>
#include <stdio.h>


#define MAX_KEY_VALUE_LENGTH	2048	//	includes terminating zero


//	Each dictionary entry will be a key-value pair,
//	kept on a NULL-terminated singly-linked list.
typedef struct LocalizedPhrase
{
	Char16					*itsKey;		//	zero-terminated UTF-16 string
	Char16					*itsValue;		//	zero-terminated UTF-16 string
	struct LocalizedPhrase	*itsNextPhrase;	//	next entry in the NULL-terminated singly-linked list
} LocalizedPhrase;

//	We'll parse dictionary files with the following format:
//
//		//	This is a comment.
//		"sample key #1" = "sample value #1"
//		"sample key #2" = "sample value #2"
//
//	Within a quoted string, we'll support the escape sequences
//	\", \t, \r and \n.  Otherwise \<character> is interpreted
//	as <character>, including the special case \\ which gives
//	a single \ character.  Tabs, return and newlines may also
//	be entered directly.
typedef enum
{
	ParseExpectingKey,
	ParseExpectingKeyWithCommentBeginning,			//	one '/' received
	ParseExpectingKeyWithCommentBegun,				//	second '/' received
	ParseInKey,
	ParseInKeyWithEscapePending,					//	Lets parser read "key with\"internal quotes\"" correctly.
	ParseExpectingEqualSign,
	ParseExpectingEqualSignWithCommentBeginning,	//	one '/' received
	ParseExpectingEqualSignWithCommentBegun,		//	second '/' received
	ParseExpectingValue,
	ParseExpectingValueWithCommentBeginning,		//	one '/' received
	ParseExpectingValueWithCommentBegun,			//	second '/' received
	ParseInValue,
	ParseInValueWithEscapePending					//	Lets parser read "value with\"internal quotes\"" correctly.
} ParseStatus;


//	The platform-dependent code will initialize the language
//	whether or not it can accommodate one of the user's 
//	preferred languages.  English is the fallback.
static Char16			gLanguageCode[3]	= u"--";	//	For example "en" or "ja",
														//		or "--" if no language is present.
														//	Terminating zero is always present, for robustness.
static LocalizedPhrase	*gDictionary		= NULL;


static void			SetUpLocalizedDictionary(unsigned int aDictionarySourceLength, const Byte *aDictionarySource);
static void			ShutDownLocalizedDictionary(void);
static inline bool	SameStringInline(const Char16 *aStringA, const Char16 *aStringB);


void SetCurrentLanguage(
	const Char16	aTwoLetterLanguageCode[3])	//	For example "en" or "ja";
												//	pass "--" to clear the language
												//	and free gDictionary's memory.
{
	ErrorText		theErrorMessage						= NULL;
	size_t			theDictionaryFileNameBufferLength	= 0;
	Char16			*theDictionaryFileName				= NULL,
					theSuffix[]							= u"-??.txt";
	unsigned int	theFileSize							= 0;
	Byte			*theFileContents					= NULL;

	gLanguageCode[0] = aTwoLetterLanguageCode[0];
	gLanguageCode[1] = aTwoLetterLanguageCode[1];
	gLanguageCode[2] = 0;

	if (gDictionary != NULL)
		ShutDownLocalizedDictionary();
	
	if (aTwoLetterLanguageCode[0] != u'-'
	 || aTwoLetterLanguageCode[1] != u'-')
	{
		theDictionaryFileNameBufferLength =
			+ Strlen16(gLanguageFileBaseName)
			+ Strlen16(theSuffix)
			+ 1;	//	allow for terminating zero
		theDictionaryFileName = GET_MEMORY(theDictionaryFileNameBufferLength * sizeof(Char16));
		if (theDictionaryFileName == NULL)
			goto CleanUpSetCurrentLanguage;
		
		theSuffix[1] = aTwoLetterLanguageCode[0];
		theSuffix[2] = aTwoLetterLanguageCode[1];

		Strcpy16(theDictionaryFileName, theDictionaryFileNameBufferLength, gLanguageFileBaseName);
		Strcat16(theDictionaryFileName, theDictionaryFileNameBufferLength, theSuffix			);

		//	Note:  The dictionary file's contents are stored as UTF-8 to avoid
		//	the little-endian/big-endian problems that a UTF-16 file would cause.
		//
		theErrorMessage = GetFileContents(	u"Languages",
											theDictionaryFileName,
											&theFileSize,
											&theFileContents);
		if (theErrorMessage != NULL)
			goto CleanUpSetCurrentLanguage;

		SetUpLocalizedDictionary(theFileSize, theFileContents);
	}

CleanUpSetCurrentLanguage:

	FreeFileContents(&theFileSize, &theFileContents);
	
	FREE_MEMORY_SAFELY(theDictionaryFileName);

	if (theErrorMessage != NULL)
		ErrorMessage(theErrorMessage, u"Internal error in SetCurrentLanguage()");
}

const Char16 *GetCurrentLanguage(void)
{
	return gLanguageCode;
}

bool IsCurrentLanguage(const Char16 aTwoLetterLanguageCode[3])
{
	return (aTwoLetterLanguageCode[0] == gLanguageCode[0]
		 && aTwoLetterLanguageCode[1] == gLanguageCode[1]);
}

bool CurrentLanguageReadsLeftToRight(void)
{
	//	See comment in CurrentLanguageReadsRightToLeft() below.

	return ! CurrentLanguageReadsRightToLeft();
}

bool CurrentLanguageReadsRightToLeft(void)
{
	//	The iOS versions of the various Geometry Games
	//	no longer use CurrentLanguageReadsRightToLeft()
	//	or CurrentLanguageReadsLeftToRight() at all.
	//	Only the Torus Games keeps an "app language"
	//	that can differ from the iOS system language.
	//	All the other Geometry Games run in the system language.
	//	Even in the Torus Games, the user interface layout direction
	//	follows the overall system layout direction.
	//	For example, if a user whose system language is Arabic
	//	switches the Torus Games "app language" to French,
	//	the user interface layout will remain right-to-left,
	//	even though all text (including the Crossword puzzles,
	//	the Word Search puzzles, the Help pages and the toolbar items)
	//	will be in French.

	return ((gLanguageCode[0] == u'a' && gLanguageCode[1] == u'r')		//	Arabic  (ar)
		 || (gLanguageCode[0] == u'f' && gLanguageCode[1] == u'a')		//	Persian (fa)
		 || (gLanguageCode[0] == u'h' && gLanguageCode[1] == u'e'));	//	Hebrew  (he)
}


static void SetUpLocalizedDictionary(
	unsigned int	aDictionarySourceLength,	//	Number of bytes (not Unicode characters!).
	const Byte		*aDictionarySource)			//	Dictionary source straight from file, as UTF-8 (never UTF-16).
{
	ErrorText		theErrorMessage		= NULL;
	unsigned int	theTextStart		= 0;
	ParseStatus		theParseStatus;
	char			theKeyAsUTF8[MAX_KEY_VALUE_LENGTH]		=  "";	//	zero-terminated UTF-8 string
	Char16			theKeyAsUTF16[MAX_KEY_VALUE_LENGTH]		= u"";	//	zero-terminated UTF-16 string
	char			theValueAsUTF8[MAX_KEY_VALUE_LENGTH]	=  "";	//	zero-terminated UTF-8 string
	Char16			theValueAsUTF16[MAX_KEY_VALUE_LENGTH]	= u"";	//	zero-terminated UTF-16 string
	Byte			theByte					= 0;	//	for brevity
	size_t			theKeyAsUTF8Length		= 0,	//	number of bytes (including terminating zero), not number of Unicode characters
					theKeyAsUTF16Length		= 0,	//	number of UTF-16 characters (including terminating zero)
					theValueAsUTF8Length	= 0,	//	number of bytes (including terminating zero), not number of Unicode characters
					theValueAsUTF16Length	= 0;	//	number of UTF-16 characters (including terminating zero)
	LocalizedPhrase	*theNewPhrase		= NULL;
	unsigned int	i,
					j;
	
	//	We don't expect a byte-order mark (BOM) but if the end user
	//	edits the language file and saves it with a BOM, we want
	//	to handle it gracefully.
	if (aDictionarySourceLength >= 3
	 && aDictionarySource[0] == 0xEF	//	UTF-8 encoding of BOM = 0xFEFF
	 && aDictionarySource[1] == 0xBB
	 && aDictionarySource[2] == 0xBF)
	{
		theTextStart = 3;
	}
	else
	{
		theTextStart = 0;
	}

	//	aDictionarySource contains UTF-8 text.
	//
	//	aDictionarySource will always be UTF-8, never UTF-16,
	//	because UTF-8 files never have byte-order issues.
	//	By contrast, a UTF-16 file would need to be encoded
	//	differently on little-endian and big-endian computers.
	//
	//	Because the characters defining the dictionary's
	//	structure are 7-bit ASCII characters (namely 
	//	the double quotation marks " and the equal sign = )
	//	we may parse the file one byte at a time,
	//	in effect temporarily ignoring the fact that keys and values
	//	may contain multibyte UTF-8 sequences.
	//
	theParseStatus = ParseExpectingKey;
	for (i = theTextStart; i < aDictionarySourceLength; i++)
	{
		theByte = aDictionarySource[i];

		switch (theParseStatus)
		{
			case ParseExpectingKey:
				switch (theByte)
				{
					case '"':
						theParseStatus		= ParseInKey;
						theKeyAsUTF8Length	= 0;
						break;

					case '/':
						theParseStatus = ParseExpectingKeyWithCommentBeginning;
						break;

					case '=':
						theErrorMessage = u"Encountered '=' while expecting a key.";
						goto CleanUpSetUpLocalizedDictionary;

					case '\n':
					case '\r':
					case ' ':
					case '\t':
						//	White space is legal.  Ignore it.
						break;

					case ';':
						theErrorMessage = u"Encountered an unnecessary semicolon ';' .  This program's dictionary format does not require them (unlike Macintosh .strings files).";
						goto CleanUpSetUpLocalizedDictionary;

					default:
						theErrorMessage = u"Encountered extraneous non-commented character while expecting a key.";
						goto CleanUpSetUpLocalizedDictionary;
				}
				break;

			case ParseExpectingKeyWithCommentBeginning:
				switch (theByte)
				{
					case '/':
						theParseStatus = ParseExpectingKeyWithCommentBegun;
						break;

					default:
						theErrorMessage = u"Found first '/' beginning a comment, but not second '/'.";
						goto CleanUpSetUpLocalizedDictionary;
				}
				break;

			case ParseExpectingKeyWithCommentBegun:
				switch (theByte)
				{
					case '\n':
					case '\r':
						theParseStatus = ParseExpectingKey;
						break;

					default:
						//	Ignore contents of comment.
						break;
				}
				break;

			case ParseInKey:
				switch (theByte)
				{
					case '\\':	//	single backslash character
						theParseStatus = ParseInKeyWithEscapePending;
						break;

					case '"':
						//	Set the new ParseStatus.
						theParseStatus = ParseExpectingEqualSign;
						
						//	Prepare the terminating zero.
						theByte = 0;

						//	Fall through to the generic case
						//	to add the terminating zero to theKeyAsUTF8.
					default:
						//	Add theByte (or terminating zero from fall-through case)
						//	if there's room for it.
						if (theKeyAsUTF8Length < MAX_KEY_VALUE_LENGTH)
							theKeyAsUTF8[theKeyAsUTF8Length++] = theByte;
						else
						{
							theErrorMessage = u"One of the dictionary's keys is too long.";
							goto CleanUpSetUpLocalizedDictionary;
						}
						break;
				}
				break;

			case ParseInKeyWithEscapePending:

				switch (theByte)
				{
					//	Interpret the escape sequences \t, \n and \r.
					case 't':	theByte = '\t';	break;
					case 'n':	theByte = '\n';	break;
					case 'r':	theByte = '\r';	break;
					
					//	Leave all other characters unmodified.
					//	In particular, \" gives a " and
					//	\\ gives a \.
					default:	break;
				}
					
				//	Add theByte if there's room for it.
				if (theKeyAsUTF8Length < MAX_KEY_VALUE_LENGTH)
					theKeyAsUTF8[theKeyAsUTF8Length++] = theByte;
				else
				{
					theErrorMessage = u"One of the dictionary's keys is too long.";
					goto CleanUpSetUpLocalizedDictionary;
				}
				
				//	Clear the pending escape.
				theParseStatus = ParseInKey;

				break;

			case ParseExpectingEqualSign:
				switch (theByte)
				{
					case '/':
						theParseStatus = ParseExpectingEqualSignWithCommentBeginning;
						break;

					case '=':
						theParseStatus = ParseExpectingValue;
						break;

					case '\n':
					case '\r':
					case ' ':
					case '\t':
						//	White space is legal.  Ignore it.
						break;

					default:
						theErrorMessage = u"Encountered extraneous non-commented character while expecting an equals sign '=' .";
						goto CleanUpSetUpLocalizedDictionary;
				}
				break;

			case ParseExpectingEqualSignWithCommentBeginning:
				switch (theByte)
				{
					case '/':
						theParseStatus = ParseExpectingEqualSignWithCommentBegun;
						break;

					default:
						theErrorMessage = u"Found first '/' beginning a comment, but not second '/'.";
						goto CleanUpSetUpLocalizedDictionary;
				}
				break;

			case ParseExpectingEqualSignWithCommentBegun:
				switch (theByte)
				{
					case '\n':
					case '\r':
						theParseStatus = ParseExpectingEqualSign;
						break;

					default:
						//	Ignore contents of comment.
						break;
				}
				break;

			case ParseExpectingValue:
				switch (theByte)
				{
					case '"':
						theParseStatus			= ParseInValue;
						theValueAsUTF8Length	= 0;
						break;

					case '/':
						theParseStatus = ParseExpectingValueWithCommentBeginning;
						break;

					case '=':
						theErrorMessage = u"Encountered '=' while expecting a value.";
						goto CleanUpSetUpLocalizedDictionary;

					case '\n':
					case '\r':
					case ' ':
					case '\t':
						//	White space is legal.  Ignore it.
						break;

					default:
						theErrorMessage = u"Encountered extraneous non-commented character while expecting a value.";
						goto CleanUpSetUpLocalizedDictionary;
				}
				break;

			case ParseExpectingValueWithCommentBeginning:
				switch (theByte)
				{
					case '/':
						theParseStatus = ParseExpectingValueWithCommentBegun;
						break;

					default:
						theErrorMessage = u"Found first '/' beginning a comment, but not second '/'.";
						goto CleanUpSetUpLocalizedDictionary;
				}
				break;

			case ParseExpectingValueWithCommentBegun:
				switch (theByte)
				{
					case '\n':
					case '\r':
						theParseStatus = ParseExpectingValue;
						break;

					default:
						//	Ignore contents of comment.
						break;
				}
				break;

			case ParseInValue:
				switch (theByte)
				{
					case '\\':	//	single backslash character
						theParseStatus = ParseInValueWithEscapePending;
						break;

					case '"':
						//	Set the new ParseStatus.
						theParseStatus = ParseExpectingKey;
						
						//	Prepare the terminating zero.
						theByte = 0;

						//	Fall through to the generic case
						//	to add the terminating zero to theValueAsUTF8.
					default:
						//	Add theByte (or terminating zero from fall-through case)
						//	if there's room for it.
						if (theValueAsUTF8Length < MAX_KEY_VALUE_LENGTH)
							theValueAsUTF8[theValueAsUTF8Length++] = theByte;
						else
						{
							theErrorMessage = u"One of the dictionary's values is too long.";
							goto CleanUpSetUpLocalizedDictionary;
						}
						
						//	If we just received the terminating '"',
						//	then both theKeyAsUTF8 and theValueAsUTF8 are complete.
						//	Create a new dictionary entry.
						if (theParseStatus == ParseExpectingKey)
						{
							//	Allocate theNewPhrase itself.
							theNewPhrase = (LocalizedPhrase *) GET_MEMORY(sizeof(LocalizedPhrase));
							if (theNewPhrase == NULL)
							{
								theErrorMessage = u"Memory allocation failed (theNewPhrase).";
								goto CleanUpSetUpLocalizedDictionary;
							}

							//	Set pointers to NULL for robust error handling.
							theNewPhrase->itsKey		= NULL;
							theNewPhrase->itsValue		= NULL;
							theNewPhrase->itsNextPhrase	= NULL;

							//	Convert theKeyAsUTF8 to theKeyAsUTF16.
							//	The number of UTF-16 characters never exceeds the number of UTF-8 bytes,
							//	so there is no risk of a buffer overrun.
							if ( ! UTF8toUTF16(theKeyAsUTF8, theKeyAsUTF16, BUFFER_LENGTH(theKeyAsUTF16)) )
							{
								theErrorMessage = u"Key contains invalid UTF-8.";
								goto CleanUpSetUpLocalizedDictionary;
							}
							theKeyAsUTF16Length = Strlen16(theKeyAsUTF16) + 1;	//	+1 to include terminating zero

							//	Allocate and initialize itsKey.
							theNewPhrase->itsKey = (Char16 *) GET_MEMORY(theKeyAsUTF16Length * sizeof(Char16));
							if (theNewPhrase->itsKey == NULL)
							{
								theErrorMessage = u"Memory allocation failed (theNewPhrase->itsKey).";
								goto CleanUpSetUpLocalizedDictionary;
							}
							for (j = 0; j < theKeyAsUTF16Length; j++)	//	includes terminating zero
								theNewPhrase->itsKey[j] = theKeyAsUTF16[j];
								
							//	Convert theValueAsUTF8 to theValueAsUTF16.
							//	The number of UTF-16 characters never exceeds the number of UTF-8 bytes,
							//	so there is no risk of a buffer overrun.
							if ( ! UTF8toUTF16(theValueAsUTF8, theValueAsUTF16, BUFFER_LENGTH(theValueAsUTF16)) )
							{
								theErrorMessage = u"Value contains invalid UTF-8.";
								goto CleanUpSetUpLocalizedDictionary;
							}
							theValueAsUTF16Length = Strlen16(theValueAsUTF16) + 1;	//	+1 to include terminating zero

							//	Allocate and initialize itsValue.
							theNewPhrase->itsValue = (Char16 *) GET_MEMORY(theValueAsUTF16Length * sizeof(Char16));
							if (theNewPhrase->itsValue == NULL)
							{
								theErrorMessage = u"Memory allocation failed (theNewPhrase->itsValue).";
								goto CleanUpSetUpLocalizedDictionary;
							}
							for (j = 0; j < theValueAsUTF16Length; j++)	//	includes terminating zero
								theNewPhrase->itsValue[j] = theValueAsUTF16[j];

							//	theNewPhrase is complete.
							//	Install it into gDictionary.
							theNewPhrase->itsNextPhrase	= gDictionary;
							gDictionary					= theNewPhrase;
							theNewPhrase				= NULL;
						}
		
						break;
				}
				break;

			case ParseInValueWithEscapePending:

				switch (theByte)
				{
					//	Interpret the escape sequences \t, \n and \r.
					case 't':	theByte = '\t';	break;
					case 'n':	theByte = '\n';	break;
					case 'r':	theByte = '\r';	break;
					
					//	Leave all other characters unmodified.
					//	In particular, \" gives a " and
					//	\\ gives a \.
					default:	break;
				}
					
				//	Add theByte if there's room for it.
				if (theValueAsUTF8Length < MAX_KEY_VALUE_LENGTH)
					theValueAsUTF8[theValueAsUTF8Length++] = theByte;
				else
				{
					theErrorMessage = u"One of the dictionary's values is too long.";
					goto CleanUpSetUpLocalizedDictionary;
				}
				
				//	Clear the pending escape.
				theParseStatus = ParseInValue;

				break;
			
			default:
				theErrorMessage = u"Internal Error:  theParseStatus has an illegal value.";
				goto CleanUpSetUpLocalizedDictionary;
		}
	}
	
	//	Did the file end at a reasonable place?
	if (theParseStatus != ParseExpectingKey
	 && theParseStatus != ParseExpectingKeyWithCommentBegun)
	{
		theErrorMessage = u"Syntax error:  unexpected end-of-file.";
		goto CleanUpSetUpLocalizedDictionary;
	}

CleanUpSetUpLocalizedDictionary:
	
	if (theNewPhrase != NULL)
	{
		FREE_MEMORY_SAFELY(theNewPhrase->itsKey);
		FREE_MEMORY_SAFELY(theNewPhrase->itsValue);
		
		FREE_MEMORY_SAFELY(theNewPhrase);
	}
	
	if (theErrorMessage != NULL)
		ErrorMessage(theErrorMessage, u"Error in SetUpLocalizedDictionary()");
}


static void ShutDownLocalizedDictionary(void)
{
	LocalizedPhrase	*theDeadPhrase;

	while (gDictionary != NULL)
	{
		theDeadPhrase	= gDictionary;
		gDictionary		= gDictionary->itsNextPhrase;

		FREE_MEMORY_SAFELY(theDeadPhrase->itsKey);
		FREE_MEMORY_SAFELY(theDeadPhrase->itsValue);
		
		FREE_MEMORY_SAFELY(theDeadPhrase);
	}
}


//	Caution:  GetLocalizedText() returns a pointer to one of the values in gDictionary.
//	The caller should finish using that pointer and discard it before calling SetCurrentLanguage(),
//	because ShutDownLocalizedDictionary() will invalidate all such pointers.
const Char16 *GetLocalizedText(
	const Char16 *aKey)
{
	LocalizedPhrase	*thePhrase;

	if (aKey == NULL)
		return u"<missing key>";

	for (	thePhrase = gDictionary;
			thePhrase != NULL;
			thePhrase = thePhrase->itsNextPhrase)
	{
		if (SameStringInline(aKey, thePhrase->itsKey))
			return thePhrase->itsValue;
	}
	
	return u"<text not found>";
}


const Char16 *GetEndonym(	//	Returns 0-terminated UTF-16 string.
	const Char16	aTwoLetterLanguageCode[3])	//	For example "en" or "ja"
{
	const Char16	*c;
	
	c = aTwoLetterLanguageCode;	//	for brevity
	
	if (SameTwoLetterLanguageCode(c, u"ar")) return u"العربية";
	if (SameTwoLetterLanguageCode(c, u"cy")) return u"Cymraeg";
	if (SameTwoLetterLanguageCode(c, u"de")) return u"Deutsch";
	if (SameTwoLetterLanguageCode(c, u"el")) return u"Ελληνικά";
	if (SameTwoLetterLanguageCode(c, u"en")) return u"English";
	if (SameTwoLetterLanguageCode(c, u"es")) return u"Español";
	if (SameTwoLetterLanguageCode(c, u"et")) return u"Eesti";
	if (SameTwoLetterLanguageCode(c, u"fi")) return u"Suomi";
	if (SameTwoLetterLanguageCode(c, u"fr")) return u"Français";
	if (SameTwoLetterLanguageCode(c, u"it")) return u"Italiano";
	if (SameTwoLetterLanguageCode(c, u"ja")) return u"日本語";
	if (SameTwoLetterLanguageCode(c, u"ko")) return u"한국어";
	if (SameTwoLetterLanguageCode(c, u"nl")) return u"Nederlands";
	if (SameTwoLetterLanguageCode(c, u"pt")) return u"Português";
	if (SameTwoLetterLanguageCode(c, u"ru")) return u"Русский";
	if (SameTwoLetterLanguageCode(c, u"sv")) return u"Svensk";
	if (SameTwoLetterLanguageCode(c, u"vi")) return u"Tiếng Việt";
	if (SameTwoLetterLanguageCode(c, u"zh"))	//	generic "中文"
		FatalError(u"Please replace generic Chinese “zh” with simplified Chinese “zs” and traditional Chinese “zt”.", u"Internal Error");
	if (SameTwoLetterLanguageCode(c, u"zs")) return u"简体中文";
	if (SameTwoLetterLanguageCode(c, u"zt")) return u"繁體中文";
	
	FatalError(	u"GetEndonym() received an unexpected language code.",
				u"Internal Error");
	return u"";	//	keeps the compiler happy
}


bool SameTwoLetterLanguageCode(
	const Char16	*aStringA,
	const Char16	*aStringB)
{
	return aStringA[0] == aStringB[0]
		&& aStringA[0] != 0
		&& aStringA[1] == aStringB[1]
		&& aStringA[1] != 0
		&& aStringA[2] == 0
		&& aStringB[2] == 0;
}

static inline bool SameStringInline(
	const Char16	*aStringA,
	const Char16	*aStringB)
{
	while (true)
	{
		if (*aStringA != *aStringB)
			return false;
		
		if (*aStringA == 0)
			return true;
		
		aStringA++;
		aStringB++;
	}
}
