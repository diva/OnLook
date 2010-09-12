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
#include "lldarray.h"
#include "llfile.h"
#include "llchat.h"
#include "llfloaterchat.h"

ASFloaterContactGroups* ASFloaterContactGroups::sInstance = NULL;
LLDynamicArray<LLUUID> ASFloaterContactGroups::mSelectedUUIDs;

ASFloaterContactGroups::ASFloaterContactGroups()
:	LLFloater(std::string("floater_contact_groups"), std::string("FloaterContactRect"), LLStringUtil::null)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_contact_groups.xml");
}

// static
void ASFloaterContactGroups::show(LLDynamicArray<LLUUID> ids)
{
    if (!sInstance)
	sInstance = new ASFloaterContactGroups();

	mSelectedUUIDs = ids;

    sInstance->open();
	sInstance->populateGroupList();
	sInstance->populateFriendList();

	sInstance->childSetAction("Cancel", onBtnClose, sInstance);
	sInstance->childSetAction("Save", onBtnSave, sInstance);
	sInstance->childSetAction("Create", onBtnCreate, sInstance);
	sInstance->childSetAction("Delete", onBtnDelete, sInstance);
}

void ASFloaterContactGroups::onBtnDelete(void* userdata)
{
	ASFloaterContactGroups* self = (ASFloaterContactGroups*)userdata;

	if(self)
	{
		LLScrollListCtrl* scroller = self->getChild<LLScrollListCtrl>("group_scroll_list");
		if(scroller != NULL) 
		{
			self->deleteContactGroup(scroller->getValue().asString());
			self->populateGroupList();
		}
	}
}

void ASFloaterContactGroups::onBtnSave(void* userdata)
{
	ASFloaterContactGroups* self = (ASFloaterContactGroups*)userdata;

	if(self)
	{
		if (self->mSelectedUUIDs.count() > 0)
		{
			LLScrollListCtrl* scroller = self->getChild<LLScrollListCtrl>("group_scroll_list");
			if(scroller != NULL) 
			{
				for (S32 i = self->mSelectedUUIDs.count(); i > 0; --i)
				{
					self->addContactMember(scroller->getValue().asString(), self->mSelectedUUIDs.get(i));
				}
			}
		}
	}
}

void ASFloaterContactGroups::onBtnClose(void* userdata)
{
	ASFloaterContactGroups* self = (ASFloaterContactGroups*)userdata;
	if(self) self->close();
}

void ASFloaterContactGroups::onBtnCreate(void* userdata)
{
	ASFloaterContactGroups* self = (ASFloaterContactGroups*)userdata;
	if(self)
	{
		LLLineEditor* editor = self->getChild<LLLineEditor>("add_group_lineedit");
		if (editor)
		{
			LLScrollListCtrl* scroller = self->getChild<LLScrollListCtrl>("friend_scroll_list");
			if(scroller != NULL) 
			{
				self->createContactGroup(editor->getValue().asString());
				self->populateGroupList();
			}
		}
		else
		{
			LLChat msg("Null editor");
			LLFloaterChat::addChat(msg);
		}
	}
	else
	{
		LLChat msg("Null floater");
		LLFloaterChat::addChat(msg);
	}
}

ASFloaterContactGroups::~ASFloaterContactGroups()
{
    sInstance=NULL;
}

void ASFloaterContactGroups::populateFriendList()
{
	LLScrollListCtrl* scroller = getChild<LLScrollListCtrl>("friend_scroll_list");
	if(scroller != NULL) 
	{
		
	}
}

std::string ASFloaterContactGroups::cleanFileName(std::string filename)
{
	std::string invalidChars = "\"\'\\/?*:<>| ";
	S32 position = filename.find_first_of(invalidChars);
	while (position != filename.npos)
	{
		filename[position] = '_';
		position = filename.find_first_of(invalidChars, position);
	}
	return filename;
}

void ASFloaterContactGroups::addContactMember(std::string contact_grp, LLUUID to_add)
{
	std::string clean_contact_grp = cleanFileName(contact_grp);
	std::string member_string = "Ascent." + clean_contact_grp + ".Members";

	if (!gSavedSettings.controlExists(member_string)) 
		gSavedSettings.declareString(member_string, to_add.asString(), "Stores UUIDs of members for " + clean_contact_grp, TRUE);
	else
		gSavedSettings.setString(member_string, gSavedSettings.getString(member_string) + " " + to_add.asString());
}

void ASFloaterContactGroups::deleteContactGroup(std::string contact_grp)
{
	std::string clean_contact_grp = cleanFileName(contact_grp);
	std::string display_string = "Ascent." + clean_contact_grp + ".Display";
	gSavedSettings.setString(display_string, "##DELETED##");

	std::string group_list(gSavedSettings.getString("AscentContactGroups"));

	if (group_list.find(clean_contact_grp) != group_list.npos)
	{
		gSavedSettings.setString(
			"AscentContactGroups", 
			group_list.erase(
				group_list.find_first_of(clean_contact_grp), 
				clean_contact_grp.length()
			)
		);
	}
}

void ASFloaterContactGroups::createContactGroup(std::string contact_grp)
{
	std::string clean_contact_grp = cleanFileName(contact_grp);
	std::string display_string = "Ascent." + clean_contact_grp + ".Display";
	std::string member_string = "Ascent." + clean_contact_grp + ".Members";

	if (gSavedSettings.controlExists(display_string)) 
	{
		std::string s_display = gSavedSettings.getString(display_string);
		if (s_display != "##DELETED##")
		{
			LLChat msg("Can't create duplicate group names.");
			LLFloaterChat::addChat(msg);
			return;
		}
	}

	gSavedSettings.declareString(member_string, "NULL_KEY", "Stores UUIDs of members for " + clean_contact_grp, TRUE);
	gSavedSettings.declareString(display_string, contact_grp, "Stores the real name of " + clean_contact_grp, TRUE);

	gSavedSettings.setString(display_string, contact_grp);
	gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), TRUE);	

	gSavedSettings.setString(member_string, "NULL_KEY");
	gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), TRUE);	

	if (!gSavedSettings.controlExists("AscentContactGroups")) 
	{
		gSavedSettings.declareString("AscentContactGroups", clean_contact_grp, "Stores titles of all Contact Groups", TRUE);
		gSavedSettings.setString("AscentContactGroups", clean_contact_grp);
	}
	else
	{
		std::string group_list(gSavedSettings.getString("AscentContactGroups"));
		group_list += " " + clean_contact_grp;
		gSavedSettings.setString("AscentContactGroups", group_list);
	}

	gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), TRUE);	
}

void StringSplit(std::string str, std::string delim, std::vector<std::string> results)
{
	int cutAt;
	while((cutAt = str.find_first_of(delim)) != str.npos)
	{
		if(cutAt > 0)
		{
			results.push_back(str.substr(0,cutAt));
		}
		str = str.substr(cutAt+1);
	}

	if(str.length() > 0)
	{
		results.push_back(str);
	}
}

void ASFloaterContactGroups::populateGroupList()
{
	LLScrollListCtrl* scroller = getChild<LLScrollListCtrl>("group_scroll_list");
	if(scroller != NULL) 
	{
		scroller->deleteAllItems();

		if (gSavedSettings.controlExists("AscentContactGroups")) 
		{
			scroller->addSimpleElement(gSavedSettings.getString("AscentContactGroups"), ADD_BOTTOM);
			/**
			std::vector<std::string> group_list;
			StringSplit(gSavedSettings.getString("AscentContactGroups"), " ", group_list);

			for (S32 i=0; i < group_list.size(); i++) 
			{
				scroller->addSimpleElement(group_list[i], ADD_BOTTOM);
				LLChat msg("Add " + group_list[i]);
				LLFloaterChat::addChat(msg);
			}
			**/
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