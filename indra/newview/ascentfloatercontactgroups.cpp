/** 
 * @file ascentfloatercontactgroups.cpp
 * @Author Charley Levenque
 * Allows the user to assign friends to contact groups for advanced sorting.
 *
 * Created Sept 6th 2010
 * 
 * ALL SOURCE CODE IS PROVIDED "AS IS." THE CREATOR MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 *
 * HACKERS CAN TURN YOUR COMPUTER INTO A BOMB.
 *		& BLOW YOUR FAMILY TO SMITHEREENS!
 *
 *								 \         .  ./
 *							   \      .:";'.:.."   /
 *								   (M^^.^~~:.'").
 *							 -   (/  .    . . \ \)  -
 *	                            ((| :. ~ ^  :. .|))
 *	                        -   (\- |  \ /  |  /)  -
 *	    T                         -\  \     /  /-
 *	   [_]..........................\  \   /  /
*/

/*
                      __---__
                   _-       _--______
              __--( /     \ )XXXXXXXXXXXXX_
            --XXX(   O   O  )XXXXXXXXXXXXXXX-
           /XXX(       U     )        XXXXXXX\
         /XXXXX(              )--_  XXXXXXXXXXX\
        /XXXXX/ (      O     )   XXXXXX   \XXXXX\
        XXXXX/   /            XXXXXX   \__ \XXXXX----
        XXXXXX__/          XXXXXX         \__----  -
---___  XXX__/          XXXXXX      \__         ---
  --  --__/   ___/\  XXXXXX            /  ___---=
    -_    ___/    XXXXXX              '--- XXXXXX
      --\/XXX\ XXXXXX                      /XXXXX
        \XXXXXXXXX                        /XXXXX/
         \XXXXXX                        _/XXXXX/
           \XXXXX--__/              __-- XXXX/
            --XXXXXXX---------------  XXXXX--
               \XXXXXXXXXXXXXXXXXXXXXXXX-
                 --XXXXXXXXXXXXXXXXXX-
			   CODE GHOSTS OFFICIALLY BUSTED
*/

#include "llviewerprecompiledheaders.h"

#include "ascentfloatercontactgroups.h"

//UI Elements
#include "llbutton.h" //Buttons
#include "llcombobox.h" //Combo dropdowns
#include "llscrolllistctrl.h" //List box for filenames
#include "lluictrlfactory.h" //Loads the XUI

// project includes
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llsdserialize.h"

#include "llchat.h"
#include "llfloaterchat.h"

ASFloaterContactGroups* ASFloaterContactGroups::sInstance = NULL;

ASFloaterContactGroups::ASFloaterContactGroups()
:	LLFloater(std::string("floater_contact_groups"), std::string("FloaterContactRect"), LLStringUtil::null)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_contact_groups.xml");
}

// static
void ASFloaterContactGroups::show(void*)
{
    if (!sInstance)
	sInstance = new ASFloaterContactGroups();

    sInstance->open();
	sInstance->populateGroupList();
}

ASFloaterContactGroups::~ASFloaterContactGroups()
{
    sInstance=NULL;
}

void ASFloaterContactGroups::populateGroupList()
{
	LLScrollListCtrl* scroller = getChild<LLScrollListCtrl>("group_scroll_list");
	if(scroller != NULL) 
	{
		std::string name;

		gDirUtilp->getNextFileInDir(gDirUtilp->getPerAccountChatLogsDir(),"*",name,false);//stupid hack to clear last file search
		scroller->clear();

		std::string path_name(gDirUtilp->getPerAccountChatLogsDir() + gDirUtilp->getDirDelimiter());
		S32 count = gDirUtilp->countFilesInDir(path_name, "*.grp");
		bool found = true;	
		LLChat msg(count + " total files in folder " + path_name);
		LLFloaterChat::addChat(msg);
		while(found = gDirUtilp->getNextFileInDir(path_name, "*.grp", name, false)) 
		{
			if ((name == ".") || (name == "..")) continue;

			//llinfos << "path name " << path_name << " and name " << name << " and found " << found << llendl;
			if(found)
			{
				LLChat msg("Found: " + name);
				LLFloaterChat::addChat(msg);
				scroller->addSimpleElement(name, ADD_BOTTOM);
			}
			else
			{
				LLChat msg("Skipped: " + name);
				LLFloaterChat::addChat(msg);
			}
		}
	} 
	else
	{
		LLChat msg("Null Scroller");
		LLFloaterChat::addChat(msg);	
	}
}

/*
*                                   BATGIRL
*
*
*             .//////.....     I:            .
*      .////....///////........IA......     //
*    XYt///////./.....////......$$$$$$$$$$$./I$
* .Y/XX//?//./....////////......$$$$$$$$$$$$$$/
* XYXXXXXXX///......//////.....$$$$$$$$$$$$$$$.
* XXXXXXXX////XX/....//.///./..$$$$$$$$$$$$$$$....
* ///////......////X...//.//./$$$$$$$$$$$$$$$...../..
* ////..////////..../X/X//////$$$-.$$$$$$$$$$./...///..
* ///////////////....//X/XX///$$ .'$$.$-..$$$.///..//?....
* ////////////X/////////XX/XX/$I   .;;.  .$$.//xX/XXXXX///////. .$$$$$$/
* /XX/XXX//////XXXXX////XXX/X/$$ .::-   ..$//XXX/////////////$$$$$$$/
* ////XXX//XXXXXX//////XXXXX//$$   ---   $/////X///XXXXXXXA$$$$$$../...
* /////XX/XX$$$$$X$$$$$$$////$$$$.    .$$$$$X//$$$$$$X/X/$$$$(////..///.
* ///XXXXX$$$$$$$$$$$$$X$$///$$$$$$$$$$$$$$$XX$$$$$X//////$$$$$$////////
* ?$$$$$$$$$$$$$:''':.::'V$$$$$'$$$$$$XXX$$$$X''''$$XXXX/$A$$$I,X//XX/////
* $$$$$$$$$.... ..    .:.::::V$$$$$$$$$$::..       .$$$:X//VA$H/X/XX////
* $$$$$$$$::. . .     ..:I::. . .:..I:.             ..$$$$XHAMMHIHA//XX/
* $$$$$$$:..:...     ..I:.     .::.       .          .$$$$VMMMMMWV'I:.XX
* $$$$$$$:. .:$$$$$$$$.       .I:.         .:..         .$$$VMMMMV/.//XXXX
* $$$$$$::.   $$4$$$$. now   .I.  .:. with  ..:$;..        .$$VMMMA/XXXXX
* $$$$$$A:.  /$$$$$$:dynamic.I.. .:..:breast .:$$$$:.       .AMMMM$XXXX//
* $$$$$IMMAAM$$$$$$::       .I..  ...       .:$$$$$$$:.    .AMMMMM'$$XX$
* $$$$$IMMHMV$$$$$$$.  phys.:I...ics         .$$$$$$$$$$:. ..VMMMMMX$$$:X
* $$$$$:MHHM$$$$$$$$$:.. ...:II... .  ...   :$$$$$$$$$$$$$:.:VMMMM'$$$$$
* $$$$$HMHMV$$$$$$$$$$;....:.:II:......-   .$$$$$$$$$$$$$$$$:.:VM'$$$$$$
* $$$$4HMHMV$$$$$$$$$$;....:.:II:......-   .$$$$$$$$$$$$$$$$:.:VM'A$$$$$
* $$$$$HMHM4$$$$$$$$$$$$;. .:::.::..    .:$$$$$$$$$$$$$$$$$$$$$$$V$$$$$$
* $$$$$MHHV$$$$$$$$$$$$:.    .:.         .:$MV'.VMM$$$$$$$$$$$$$$$$$$$$$
* $$$$$MMM$$MM$$$MM$$$$...    ..         :$$.    .VM$$$$$$MMMM$M$$$$$$$$
* $$$$$MMHA$$MMMM$M4$V  ..    ...       .:$:      .VM$$$$MMMMV..VM$$M$M$
* $$$$$MMLMA$MMMMMM$$'  ...    ...      :$$.       .H$$$$MV'    .VMM$M$$
* $$$$$MHAHH$V.  .V$$.  ...     ...    .:$$         V$$$MV        VMM$MM
* $$$$$MMMMHA      V$   :.      . .    ..'V.         V$$V          VMM$$
* $$$$$$VHMHA       $  .:..    .:       .  $.        .$$            VMM$
* $$$$$$VAA        . .::..    .;       :..  .        $$             VMM$$
* $$$$$$$$$$         .::. .    ..       .  .          V.              VM
* $$$$$M$$$$.       ..:.:...   .       .   ..         I               .V
* $$$$$$M$$$$      . .:. :...     .    .. ..          .                .
* $$$$$$$$$$$.     . .  .:..     .  .::.   .
* $$$V''$$M$$$     :                       .
* $$$     .VM$.  .:.           $$$$        .
* $$$       $$$   :.         $$$$$$$$$
* $$$$       $$   ..          $$$$$$$       .
* $$$$$      IV   ..           $$$$$
* $$$$$$.    $   ..            .$$$.          .
* M$$$$$$$   .    ...          .: :...
* MM$$$$$$$       ...          .. :...        .
* .V$MM$$$4$       ...         .: .::..
*   .V$MM$$$$        ..         .$.:...        .
*    .V$MM$$$$      ...         :$$$::..       .
*      V$:V$:$$     ...         .$$$::..       .
*       $  $$ ;$     ..         .$$$$:....     .
*       .   V$ .$    ...        .$$$$.:..
*            V$  $    ..        .$$$$$:.. .    .
*             V$  .    ..       .  $$$::..     .
*              V$      ...      .  $$$;:.
*               $I      ..      .   $$;:..     .
*                $.     ...    ..    $;;..     .
*                 $     ...   ...     $;:..  ..
*                  $    ...    ..       $$.::$
*                   I.   ...    ..        $$$$
*                    .   ...   ...
*                       .:... .:.
*                       ::..  ...
*                      .:..   .:.
*                      :..   .::.
*                      :.    .:..
*                      :.  .$$$;.
*                      :. $$$$$$.
*                      :$$$$$$$$$
*                      $4$$$$$$$$
*                      $$$$$$$$$$
*                      ;$$$$$M$$;
*                      $$$$$IM$$$
*                      V$$$M$$$$$
*                      ;$$$$M$$$;
*                       $$$M$$$$;
*                       $$$$$$$$
*                       ;$$$$$$$
*                        $$$$$$$
*                        $$$$$$$
*                        $$$$$$$
*                        ;$$$$$$
*                         $$$$$$
*                         $$$$$$.
*                        $$$$$$$$
*                        $$$$$$$:
*                        $$$$$$$:
*                        :$$$$$$:
*                       $$$$$$$:
*                       $$$$$$$$
*                       $$$$$$$$
*                        $$$$$$
*                         $$$$
*                          $$
*
*
*/