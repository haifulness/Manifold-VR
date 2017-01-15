//	GeometryGames-Win32-WinMain.c
//
//	Provides supporting functions for WinMain()
//	in the Win32 versions of the various Geometry Games.
//
//	© 2016 by Jeff Weeks
//	See TermsOfUse.txt

#include "GeometryGames-Win32.h"
#include "GeometryGamesIconID.h"
#include "GeometryGamesUtilities-Win.h"
#include "GeometryGamesLocalization.h"
//#include <stdio.h>	//	for swprintf()

//	The MinGW implementations of swprintf() and vswprintf()
//	differ from the ISO standard versions in that
//	they lack a buffer-length parameter.
extern int __cdecl __MINGW_NOTHROW swprintf(wchar_t*, const wchar_t*, ...);

	
//	How many pixels of empty screen space should
//	we allow around the default window position?
#define WINDOW_MARGIN	16

//	By how much should we stagger successive windows?
#define WINDOW_STAGGER	32

//	Minimum macro
#define MIN(A,B)	((A) < (B) ? (A) : (B))


//	An "atom" uniquely identifies the main window class.
static ATOM	gMainWindowClass = 0;

//	See comments accompanying gShowFrameRate's declaration in "GeometryGames-Win32.h".
bool		gShowFrameRate = false;


void TestSupportingFiles(
	WORD	aPrimaryLangID)
{
	static WCHAR
	*theTitleEN		= u"Can’t find language file",
	*theMessageEN	=
		u"This application comes with a collection of supporting files "
		u"for Languages, Textures, Help, and so on.  When running on Microsoft Windows, "
		u"the application may fail to find these files for one of two reasons:\n"
		u"\n"
		u"Possible cause #1.\n"
		u"\n"
		u"Some older low-quality un-zipping programs “flatten the folder structure”.  "
		u"That is, they place all the un-zipped files into a single folder, "
		u"ignoring subfolders.  If this has happened to you, the language files "
		u"will be sitting in the same folder with the application itself "
		u"(the .exe file), rather than in the Languages subfolder where they belong.  "
		u"Fortunately the fix is very easy:  download a fresh copy of the application "
		u"(the .zip file) from www.geometrygames.org and then un-zip the .zip file "
		u"using Windows’s built-in un-zipper (not your third-party un-zipper).  "
		u"To use Windows’s built-in un-zipper, right-click on the .zip file "
		u"that you just downloaded and from the menu that pops up choose Extract All…  "
		u"Click “Next” as necessary to save the un-zipped application folder "
		u"to your hard drive.  The un-zipper will automatically place all files "
		u"into the correct subfolders.\n"
		u"\n"
		u"Possible cause #2.\n"
		u"\n"
		u"If you moved the application’s .exe file to your desktop or some other location "
		u"on your computer’s hard drive and you didn’t move the supporting files "
		u"along with it, then the application will fail to find its supporting files. "
		u"The fix is easy:  download a fresh copy of the application (the .zip file) "
		u"from www.geometrygames.org, un-zip it, and then move the whole folder "
		u"as a single unit, so the application and its subfolders all travel together.",

	*theTitleFR		= u"Fichier de langue non trouvé",
	*theMessageFR	=
		u"Cette application est fournie avec un ensemble de fichiers permettant d'intégrer "
		u"les langues, les textures, l’aide, etc. En tournant sous Microsoft Windows, "
		u"l'application pourrait ne pas réussir à localiser les fichiers dont elle a besoin.  "
		u"Ce problème a deux causes possibles :\n"
		u"\n"
		u"1. Certains archiveurs anciens ou de mauvaise qualité décompressent les fichiers "
		u"en éliminant la structure des répertoires, c’est-à-dire qu’ils placent "
		u"tous les fichiers décompressés dans un seul répertoire en ignorant "
		u"les sous-répertoires.  Si cela se produit, le fichier de langue est alors placé "
		u"dans le même répertoire que l’application elle-même (le fichier .exe), plutôt que "
		u"dans le sous-répertoire où il devrait être.  La solution à ce problème est "
		u"très simple :  téléchargez une copie originale de l’application compressée "
		u"(le fichier ..zip) à partir du site www.geometrygames.org et décompressez-la "
		u"en utilisant l’archiveur fourni avec Windows au lieu de votre archiveur tierce-partie. "
		u"Pour utiliser l’archiveur fourni avec Windows, faite un clic droit sur le fichier .zip "
		u"que vous venez de télécharger et choisissez “Tout extraire…” dans le menu qui apparaît.  "
		u"Cliquez sur “Suivant” autant de fois que nécessaire pour sauvegarder le répertoire "
		u"de l’application décompressée sur votre disque dur.  L’archiveur placera "
		u"automatiquement tous les fichiers dans les sous-répertoires corrects.\n"
		u"\n"
		u"2. Si vous avez déplacé le fichier .exe de l’application vers le bureau "
		u"ou vers un autre dossier de votre disque dur et que vous n’avez pas déplacé "
		u"les fichiers d’accompagnement avec lui, l’application échouera dans la recherche "
		u"de ses fichiers.  Dans ce cas, téléchargez une copie originale de l’application "
		u"(le fichier .zip) à partir du site www.geometrygames.org, décompressez-le, "
		u"et déplacez le répertoire entier, comme une unité unique. Ainsi l’application "
		u"et ses sous-répertoires seront déplacés ensemble.",

	*theTitleIT		= u"File di linguaggio non trovato",
	*theMessageIT	=
		u"Quest’applicazione è dotata di una serie di file di supporto per le lingue, "
		u"le didascalie, i testi di aiuto, etc.  Sotto Microsoft Windows può accadere "
		u"che l’applicazione non trovi questi file.  Le cause possibili sono le seguenti:\n"
		u"\n"
		u"Possibile causa #1.\n"
		u"\n"
		u"Alcuni vecchi (e cattivi) programmi di decompressione “appiattiscono” "
		u"la struttura a cartelle, ovvero mettono tutti i file estratti in una stessa "
		u"cartella, ignorando le sottocartelle.  Se questo è ciò che è successo a te, "
		u"i file di linguaggio si troveranno nella stessa cartella in cui sta "
		u"l’applicazione (file .exe), piuttosto che nella sottocartella Languages "
		u"dove dovrebbero stare.  Fortunatamente il rimedio è molto semplice:  "
		u"scarica nuovamente l’applicazione compressa (file .zip) da www.geometrygames.org "
		u"e decomprimi il file .zip usando il decompressore proprio di Windows "
		u"(non l’altro che possiedi).  Per usare il decompressore proprio di Windows "
		u"fai clic destro sul file .zip appena scaricato e seleziona “Estrai tutto…” "
		u"dal menù che appare.  Fai clic su “Continua” quante volte è necessario "
		u"per salvare la cartella dell’applicazione sul tuo disco fisso.  "
		u"Il decompressore sistemerà automaticamente i file nelle sottocartelle opportune.\n"
		u"\n"
		u"Possibile causa #2.\n"
		u"\n"
		u"Se hai spostato il file .exe dell’applicazione sul desktop o in qualche "
		u"altra posizione sul tuo disco fisso, senza però spostare insieme a esso "
		u"i file di supporto, allora l’applicazione non riuscirà a trovare i file "
		u"di supporto. La soluzione è semplice:  scarica nuovamente l’applicazione "
		u"compressa (file .zip) da www.geometrygames.org, decomprimila e poi sposta "
		u"l’intera cartella tutta in una volta, in modo che l’applicazione e le sue "
		u"sottocartelle “viaggino insieme”.",

	*theTitleJA		= u"言語ファイルが見つかりません",
	*theMessageJA	=
		u"このアプケーションは、Languages、Textures、Helpなど、複数のサポート・ファイルを必要とし、"
		u"それらが同梱されていますが、Microsoft Windows 上で起動したとき、次のような理由で、"
		u"アプリケーションが、サポート・ファイルを見つけられないことがあります。\n"
		u"\n"
		u"原因１：\n"
		u"圧縮されたファイルを展開するプログラムで、古く、質の低いものは、フォルダの階層構造を"
		u"「平坦化」してしまうものがあります。つまり、サブ・フォルダの存在を無視し、全ての展開されたファイルを、"
		u"ひとつのフォルダに入れてしまうのです。これが起こると、言語ファイルは、本来の格納場所である"
		u"言語サブ・フォルダにではなく、アプリケーション・ファイル（.exe ファイルのこと）そのものと、"
		u"同じフォルダに入れられてしまいます。幸い、この問題は、簡単に解決できます："
		u"先ず、www.geometrygames.org より、改めてアプリケーションの圧縮ファイル（.zip ファイルのこと）を"
		u"ダウンロードしてください。そして（前回使った、サード・パーティ展開プログラムではなく）、Windows に"
		u"内蔵された、純正展開プログラムで展開してください。Windows 内蔵の展開プログラムを使うには、"
		u"ダウンロードした .zip ファイルを右クリックし、メニューから「すべて展開…」を選択します。"
		u"続いて「次へ」を必要回数クリックして、展開されたアプリケーション・フォルダを、ハード・ドライブに"
		u"保存します。こうすると、全てのファイルは、それぞれの正しいサブ・フォルダに、自動的に割り振られます。\n"
		u"\n"
		u"原因２：\n"
		u"アプリケーションを、デスクトップなど、もとのフォルダ以外の場所に移動したいとき、 .exe ファイルだけを"
		u"取り出して移動させると、アプリケーションは、サポート・ファイルを見つけられなくなります。"
		u"この問題を解決するには、改めてアプリケーションの圧縮ファイル（.zip ファイルのこと）を "
		u"www.geometrygames.org よりダウンロードして展開し、全てのサブ・フォルダがアプリケーションと"
		u"一緒に移動するように、フォルダ全体を、フォルダごと移動してください。",

	*theTitlePT		= u"Ficheiro de língua não encontrado",
	*theMessagePT	=
		u"Este programa vem com uma colecção de ficheiros de apoio para Línguas, Texturas, Ajuda, etc.  "
		u"Ao usar Windows da Microsoft, o programa pode não conseguir encontrar estes ficheiros "
		u"por uma das duas razões seguintes:\n"
		u"\n"
		u"Causa possível #1.\n"
		u"\n"
		u"Alguns programas mais antigos de baixa qualidade usados para descomprimir ficheiros "
		u"não recuperam “a estrutura das pastas”.  Isto é, eles colocam todos os ficheiros "
		u"descomprimidos numa única pasta, ignorando as sub-pastas.  Se foi isto que lhe "
		u"aconteceu, os ficheiros da língua estarão colocados na mesma pasta do próprio programa "
		u"(o ficheiro .exe), em vez de estarem colocados na sub-pasta Languages, na qual deveriam "
		u"estar. Felizmente, a solução é muito fácil: importe uma cópia recente do ficheiro "
		u"do programa (ficheiro .zip) a partir de www.geometrygames.org e depois descomprima "
		u"este ficheiro .zip usando o descompressor incluído no Windows (e não o seu descompressor "
		u"de outra proveniência).  Para usar o descompressor incluído no Windows, clique "
		u"com o botão direito do rato no ficheiro .zip que acabou de importar e, no menu "
		u"que aparece, escolha “Extrair todos…” ou “Abrir com : Pasta comprimida (zipada)”.  "
		u"Clique “Seguinte” as vezes que forem necessárias para guardar a pasta descomprimida "
		u"do programa no seu disco.  O programa de descompressão colocará automaticamente "
		u"todos os ficheiros nas sub-pastas correctas.\n"
		u"\n"
		u"Causa possível #2.\n"
		u"\n"
		u"Se deslocou o ficheiro .exe do programa, para o seu ambiente de trabalho (desktop) "
		u"ou para outro local no disco do seu computador e não deslocou os ficheiros de apoio "
		u"juntamente com ele, o programa não conseguirá encontrar os ficheiros de apoio.  "
		u"A solução é fácil: importe uma cópia recente do programa (o ficheiro .zip) a partir "
		u"de www.geometrygames.org, descomprima-o e depois desloque a pasta como um todo, "
		u"por forma a que o programa e as suas sub-pastas sejam deslocados juntos.";
										
	//	Test for the existence of an arbitrarily chosen file or directory.
	//	Presumably all the supporting files will be present, or none of them will be.
	if (GetFileAttributes(u"Languages") == INVALID_FILE_ATTRIBUTES)
	{
		//	If the test file isn't present, display a message,
		//	if possible in the user's preferred language.
		switch (aPrimaryLangID)
		{
			case LANG_FRENCH:
				MessageBox(NULL, theMessageFR, theTitleFR, MB_OK | MB_TASKMODAL);
				break;

			case LANG_ITALIAN:
				MessageBox(NULL, theMessageIT, theTitleIT, MB_OK | MB_TASKMODAL);
				break;

			case LANG_JAPANESE:
				MessageBox(NULL, theMessageJA, theTitleJA, MB_OK | MB_TASKMODAL);
				break;

			case LANG_PORTUGUESE:
				MessageBox(NULL, theMessagePT, theTitlePT, MB_OK | MB_TASKMODAL);
				break;

			case LANG_ENGLISH:
			default:
				MessageBox(NULL, theMessageEN, theTitleEN, MB_OK | MB_TASKMODAL);
				break;
		}
	}
}


void InitLocalization(
	WORD	aLanguageID)
{
	USHORT			thePrimaryLanguageID,
					theSublanguageID,
					theLanguageID;
	const Char16	*theLanguageCode;
	unsigned int	i;

	thePrimaryLanguageID = PRIMARYLANGID(aLanguageID);
	if (thePrimaryLanguageID == LANG_CHINESE)
	{
		//	Replace the Chinese sublanguage with either
		//	SUBLANG_CHINESE_SIMPLIFIED or SUBLANG_CHINESE_TRADITIONAL.
		switch (SUBLANGID(aLanguageID))
		{
			case SUBLANG_CHINESE_TRADITIONAL:
			case SUBLANG_CHINESE_HONGKONG:
			case SUBLANG_CHINESE_MACAU:
				theSublanguageID = SUBLANG_CHINESE_TRADITIONAL;
				break;

			case SUBLANG_CHINESE_SIMPLIFIED:
			case SUBLANG_CHINESE_SINGAPORE:
			default:
				theSublanguageID = SUBLANG_CHINESE_SIMPLIFIED;
				break;
		}
	}
	else
	{
		//	For languages other than Chinese, ignore the sublanguage.
		//	For example, we'll provide a single English localization
		//	for SUBLANG_ENGLISH_US, SUBLANG_ENGLISH_UK, SUBLANG_ENGLISH_AUS, etc.
		//	a single French localization for SUBLANG_FRENCH, SUBLANG_FRENCH_BELGIAN
		//	SUBLANG_FRENCH_CANADIAN, SUBLANG_FRENCH_SWISS, etc.,
		//	and similarly for all other languages except Chinese.
		theSublanguageID = SUBLANG_DEFAULT;
	}
	theLanguageID = MAKELANGID(thePrimaryLanguageID, theSublanguageID);
	
	theLanguageCode = NULL;
	
	for (i = 0; i < gNumLanguages; i++)
		if (theLanguageID == GetWin32LangID(gLanguages[i]))
			theLanguageCode = gLanguages[i];

	if (theLanguageCode == NULL)
	{
		ErrorMessage(u"Sorry, but this software does not yet support your preferred language.  It will launch in English instead.  You may, if you wish, choose an alternative from the Language menu.\n\nIf you wish to translate this software into your native tongue, please contact Jeff Weeks at www.geometrygames.org/contact.html .", u"Language not supported");
		theLanguageCode = u"en";
	}

	SetCurrentLanguage(theLanguageCode);
}


bool RegisterGeometryGamesWindowClasses(void)
{
	WNDCLASS	wc;
	ATOM		theDrawingWindowClass;

	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc		= MainWndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance 		= GetModuleHandle(NULL);
	wc.hIcon			= LoadIcon(GetModuleHandle(NULL), (void *) IDI_APP_ICON);
	wc.hCursor			= NULL;
	wc.hbrBackground	= NULL;
	wc.lpszMenuName		= NULL;	//	language-specific menu will be added later
	wc.lpszClassName 	= MAIN_WINDOW_CLASS_NAME;

	gMainWindowClass = RegisterClass(&wc);

	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc		= DrawingWndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= GetModuleHandle(NULL);
	wc.hIcon			= NULL;
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= NULL;
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= DRAWING_WINDOW_CLASS_NAME;

	theDrawingWindowClass = RegisterClass(&wc);

	return (gMainWindowClass != 0
		 && theDrawingWindowClass != 0);
}

bool IsGeometryGamesMainWindow(
	HWND	aWindow)
{
	return ( GetClassLong(aWindow, GCW_ATOM) == gMainWindowClass );
}


HWND CreateGeometryGamesWindow(
	unsigned int	aMultipleH,		//	See comments below in GetInitialWindowRect() below.
	unsigned int	aMultipleV,
	unsigned int	aToolbarHeight)	//	May be 0.
{
	DWORD	theWindowStyles;
	RECT	theWindowRect;
	HMENU	theDummyMenu;
	HWND	theWindow;

	static unsigned int	theHorizontalOffset	= 0,
						theVerticalOffset	= 0;

	//	If you plan to make use of many windows,
	//	you may want to make them a smaller, fixed size.
	//	But because the Geometry Games work mainly
	//	with a single large window, the present code creates
	//	the largest window with the given aspect ratio
	//	that fits comfortably on the screen.

	//	Choose window styles.
	theWindowStyles = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	
	//	Compute the largest square window that fits comfortably on the screen.
	GetInitialWindowRect(	theWindowStyles,
							aMultipleH,
							aMultipleV,
							aToolbarHeight,
							&theWindowRect);

	//	Create a temporary menu bar, which will
	//	1.	occupy space at the top of the window, thus
	//		facilitating correct layout of the client area, and
	//	2.	give RefreshLanguage() an "old menu" to delete,
	//		thus avoiding risk of spurious errors.
	theDummyMenu = CreateMenu();
	AppendMenu(theDummyMenu, MF_STRING, 0, u"temp");

	//	Create the window.
	theWindow = CreateWindow(
		MAIN_WINDOW_CLASS_NAME,
		u"Temporary Title",
		theWindowStyles,
		theWindowRect.left + theHorizontalOffset,
		theWindowRect.top  + theVerticalOffset,
		theWindowRect.right - theWindowRect.left,
		theWindowRect.bottom - theWindowRect.top,
		NULL, theDummyMenu, GetModuleHandle(NULL), NULL);

	//	Give up if we didn't get a window.
	if (theWindow == NULL)
		return NULL;
	
	//	Let the application replace the temporary title and menu bar
	//	with real ones, and adjust the mirroring if necessary.
	RefreshLanguage(theWindow, 0);

	//	Stagger the window for next time.
	theHorizontalOffset += WINDOW_STAGGER;
	if (theHorizontalOffset > (unsigned int) GetSystemMetrics(SM_CXSCREEN)/4)
		theHorizontalOffset = 0;
	theVerticalOffset += WINDOW_STAGGER;
	if (theVerticalOffset > (unsigned int) GetSystemMetrics(SM_CYSCREEN)/4)
		theVerticalOffset = 0;

	return theWindow;
}

void GetInitialWindowRect(
	DWORD			aWindowStyles,		//	input
	unsigned int	aMultipleH,			//	input, see comments below
	unsigned int	aMultipleV,			//	input
	unsigned int	aToolbarHeight,		//	input, may be 0
	RECT			*aBestWindowRect)	//	output
{
	RECT		theWindowRect,
				theScreenRect = {0, 0, 256, 256};	//	default in case SystemParametersInfo() fails
	signed int	theHorizontalAdjustment,
				theVerticalAdjustment,
				theClientWidth,
				theClientHeight,
				theGameWidth,
				theGameHeight,
				theWidthFactor,
				theHeightFactor,
				theFactor;

	//	How big an allowance must we make for the window frame,
	//	title bar and menu?  Start with dummy values for a 256×256
	//	window and let AdjustWindowRect() tell us how much it will change.
	SetRect(&theWindowRect, 0, 0, 256, 256);
	AdjustWindowRect(&theWindowRect, aWindowStyles, true);
	theHorizontalAdjustment	= (theWindowRect.right - theWindowRect.left) - 256;
	theVerticalAdjustment	= (theWindowRect.bottom - theWindowRect.top) - 256;

	//	How big is the visible portion of the display (excluding taskbar etc.) ?
	SystemParametersInfo(SPI_GETWORKAREA, 0, &theScreenRect, 0);

	//	Figure out the largest size window
	//	that will fit comfortably on the screen.
	theClientWidth  = (theScreenRect.right - theScreenRect.left)
					- theHorizontalAdjustment
					- 2 * WINDOW_MARGIN;
	theClientHeight = (theScreenRect.bottom - theScreenRect.top)
					- theVerticalAdjustment
					- 2 * WINDOW_MARGIN;

	//	The client area may include a toolbar as well as the main game area.
	theGameWidth	= theClientWidth;
	theGameHeight	= theClientHeight - aToolbarHeight;

	//	Make the game area a rectangle with aspect ratio
	//
	//			aMultipleH : aMultipleV
	//
	//	For example,
	//
	//		1 : 1	gives a square, as in Curved Spaces
	//		5 : 4	gives a 5:4 rectangle, as in KaleidoTile
	//		3 : 3	gives a square whose dimensions are multiples of 3,
	//					as in the Torus Games
	//		0 : 0	leaves the aspect ratio unconstrained, as in KaleidoPaint
	//
	if (aMultipleH > 0
	 && aMultipleV > 0)
	{
		theWidthFactor	= theGameWidth  / aMultipleH;	//	integer division discards remainder
		theHeightFactor	= theGameHeight / aMultipleV;	//	integer division discards remainder
		theFactor		= MIN(theWidthFactor, theHeightFactor);
		theGameWidth	= aMultipleH * theFactor;
		theGameHeight	= aMultipleV * theFactor;
	}

	//	Allow space for the toolbar.
	theClientWidth	= theGameWidth;
	theClientHeight	= theGameHeight + aToolbarHeight;

	//	Package up the final window rectangle.
	aBestWindowRect->left	= WINDOW_MARGIN;
	aBestWindowRect->top	= WINDOW_MARGIN;
	aBestWindowRect->right	= WINDOW_MARGIN + theClientWidth  + theHorizontalAdjustment;
	aBestWindowRect->bottom	= WINDOW_MARGIN + theClientHeight + theVerticalAdjustment;
}


double MeasureFramePeriod(void)
{
	//	QueryPerformanceCounter() is less reliable than one would hope.
	//	See
	//		http://blogs.msdn.com/oldnewthing/archive/2005/09/02/459952.aspx

	LARGE_INTEGER	theCurrentTime,		//	Can be inconsistent on certain dual-core systems.
					theClockFrequency;	//	Can vary depending on power management schemes.
	double			theFramePeriod;

	static bool				theSetUpFlag			= false;
	static LARGE_INTEGER	theLastRedrawTime		= {{0,0}};	//	initialize first member of union

	//	When deciding how far to advance the animation for the next frame,
	//	we face, in effect, a statistical question:  given the time required
	//	to render the last few frames, how long do we estimate it will take
	//	to render the next one?  For now we adopt the simplest possible answer:
	//	we guess that the next frame will take roughly as long as the previous
	//	one took.  In practice that approach seems to work pretty well.
	//	However, if jitteriness is a problem (for example if system activity
	//	slows one frame, in turn causing the animation to advance too far
	//	in the next frame) we could adopt a more sophisticated approach,
	//	for example we could keep track of the last three time intervals and
	//	use the middle value, which would in effect ignore any isolated anomalies.

	//	One-time initialization.
	if ( ! theSetUpFlag )
	{
		QueryPerformanceCounter(&theLastRedrawTime);
		theSetUpFlag = true;
	}

	//	How long did it take to draw the last frame?
	QueryPerformanceCounter(&theCurrentTime);
	QueryPerformanceFrequency(&theClockFrequency);
	theFramePeriod	= (double)(theCurrentTime.QuadPart - theLastRedrawTime.QuadPart)
					/ (double)(theClockFrequency.QuadPart);
	theLastRedrawTime = theCurrentTime;
	
	return theFramePeriod;
}


BOOL CALLBACK DoIdleTime(
	HWND	aWindow,
	LPARAM	anIdleTimeDataPtr)
{
	GeometryGamesWindowData		*ggwd				= NULL;
	IdleTimeData				*theIdleTimeData	= NULL;

	//	If the window is ours...
	if (IsGeometryGamesMainWindow(aWindow))
	{
		//	Prepare the input parameters.
		ggwd			= (GeometryGamesWindowData *) GetWindowLongPtr(aWindow, GWLP_USERDATA);
		theIdleTimeData	= (IdleTimeData *) anIdleTimeDataPtr;
		if (ggwd == NULL || theIdleTimeData == NULL)	//	should never occur
			return true;	//	keep going
		
		//	Let the caller know that at least one window is present.
		theIdleTimeData->itsKeepGoingFlag = true;

		//	If the window...
		if ( ! IsIconic(aWindow)				//	isn't minimized and
		 && SimulationWantsUpdates(ggwd->mdp))	//	wants an idle-time update, then...
		{
			//	Evolve its simulation over the given time period.
			SimulationUpdate(ggwd->mdp, theIdleTimeData->itsFramePeriod);

			//	Declare the window contents invalid.
			//	In effect this puts a WM_PAINT message on the message queue.
			if (ggwd->itsDrawingPanel != NULL)
				InvalidateRect(ggwd->itsDrawingPanel, NULL, false);

			//	Let the caller know that at least one animation is active.
			theIdleTimeData->itsAnimationFlag = true;
		}
	}

	return true;	//	keep going
}


void DisplayFrameRate(
	IdleTimeData	*someIdleTimeData)
{
	unsigned int	theFrameRate;
	WCHAR			*theWindowTitleFormat,
					theWindowTitle[64 + 8];	//	allow ample extra space for the digits

	static double	theTimeSinceLastDisplay	= 0.0;

	//	If the user wants to see the frame rate,
	//	write it to the window's title bar roughly once per second.
	//
	//	Note:
	//		DisplayFrameRate() shows the total time per frame.
	//		While this may be useful for some purposes,
	//		during development it may be more helpful
	//		to enable DISPLAY_GPU_TIME_PER_FRAME instead.
	//

	theTimeSinceLastDisplay += someIdleTimeData->itsFramePeriod;

	if (gShowFrameRate
	 && theTimeSinceLastDisplay >= 1.0)
	{
		theTimeSinceLastDisplay = 0.0;

		if (someIdleTimeData->itsAnimationFlag)
		{
			//	Convert the frame period to a frame rate.
			if (someIdleTimeData->itsFramePeriod > 0.0001)
			{
				theFrameRate = (unsigned int) (1.0 / someIdleTimeData->itsFramePeriod);
				theWindowTitleFormat = u"%d fps";
			}
			else	//	frame rate ≥ 10000 fps
			{
				//	Display "∞ fps".
				theFrameRate = 0;	//	will be ignored
				theWindowTitleFormat = u"∞ fps";
			}
		}
		else	//	someIdleTimeData->itsAnimationFlag == false
		{
			//	Even though we're running the idle-time code 60 times per second,
			//	we're not actually drawing anything.  So it makes more sense
			//	to display 0 fps than 60 fps.
			theFrameRate = 0;
			theWindowTitleFormat = u"%d fps";
		}
	
		//	Note:  The MinGW version of swprintf() doesn't check for buffer overruns.
		swprintf(	theWindowTitle,
					/*BUFFER_LENGTH(theWindowTitle),*/
					theWindowTitleFormat,
					theFrameRate);

		EnumThreadWindows(GetCurrentThreadId(), SetWindowTitle, (LPARAM) theWindowTitle);
	}
}


BOOL CALLBACK SetWindowTitle(
	HWND	aWindow,
	LPARAM	aTitle)	//	theTitle = (WCHAR *)aTitle, may be NULL
{
	GeometryGamesWindowData	*ggwd	= NULL;

	//	Note:  On my system the Input Method Environment (IME) creates
	//	secret windows that shadow the main window.  They are top-level windows,
	//	so EnumThreadWindows() finds them.  Specifically, each main window owns
	//	a secret window of class "IME", which in turn owns another secret window
	//	of class "MSCTFIME UI".  So we must test each window to see whether
	//	it's really one of ours.

	//	If the window is ours...
	if (IsGeometryGamesMainWindow(aWindow))
	{
		if ((WCHAR *)aTitle != NULL)	//	If the caller passed an explicit title...
		{
			//	...then show it (typically it's the frame rate).
			SetWindowText(aWindow, (WCHAR *)aTitle);
		}
		else	//	Otherwise, if the window has an associated file name...
		if ( (ggwd = (GeometryGamesWindowData *) GetWindowLongPtr(aWindow, GWLP_USERDATA)) != NULL
		   && ggwd->itsFileTitle[0] != 0)
		{
			//	...show the file name.
			SetWindowText(aWindow, ggwd->itsFileTitle);
		}
		else	//	Otherwise...
		{
			//	... set the window's default title.
			SetWindowText(aWindow, GetLocalizedText(u"WindowTitle"));
		}
	}

	return true;	//	keep going
}
