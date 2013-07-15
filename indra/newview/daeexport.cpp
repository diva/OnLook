/**
* @file daeexport.cpp
* @brief A system which allows saving in-world objects to Collada .DAE files for offline texturizing/shading.
* @authors Latif Khalifa
*
* $LicenseInfo:firstyear=2011&license=LGPLV3$
* Copyright (C) 2013 Latif Khalifa
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 3 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General
* Public License along with this library; if not, write to the
* Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301 USA */

#include "llviewerprecompiledheaders.h"

#include "daeexport.h"

//colladadom includes
#if LL_MSVC
#pragma warning (disable : 4018)
#pragma warning (push)
#pragma warning (disable : 4068)
#pragma warning (disable : 4263)
#pragma warning (disable : 4264)
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include "dae.h"
//#include "dom.h"
#include "dom/domAsset.h"
#include "dom/domBind_material.h"
#include "dom/domCOLLADA.h"
#include "dom/domConstants.h"
#include "dom/domController.h"
#include "dom/domEffect.h"
#include "dom/domGeometry.h"
#include "dom/domInstance_geometry.h"
#include "dom/domInstance_material.h"
#include "dom/domInstance_node.h"
#include "dom/domInstance_effect.h"
#include "dom/domMaterial.h"
#include "dom/domMatrix.h"
#include "dom/domNode.h"
#include "dom/domProfile_COMMON.h"
#include "dom/domRotate.h"
#include "dom/domScale.h"
#include "dom/domTranslate.h"
#include "dom/domVisual_scene.h"
#if LL_MSVC
#pragma warning (pop)
#endif

// library includes
#include "aifilepicker.h"
#include "llnotificationsutil.h"
#include "boost/date_time/posix_time/posix_time.hpp"

// newview includes
#include "lfsimfeaturehandler.h"
#include "llface.h"
#include "llvovolume.h"

// menu includes
#include "llevent.h"
#include "llmemberlistener.h"
#include "llview.h"
#include "llselectmgr.h"

extern LLUUID gAgentID;

//Typedefs used in other files, using here for consistency.
typedef LLMemberListener<LLView> view_listener_t;

namespace DAEExportUtil
{
	void saveImpl(DAESaver* daesaver, AIFilePicker* filepicker)
	{
		if (filepicker->hasFilename())
		{
			const std::string selected_filename = filepicker->getFilename();

			daesaver->saveDAE(selected_filename);
			LLNotificationsUtil::add("WavefrontExportSuccess", LLSD().with("FILENAME", selected_filename));
		}
		else llwarns << "No file; bailing" << llendl;

		delete daesaver;
	}

	void filePicker(DAESaver* daesaver, std::string name)
	{
		AIFilePicker* filepicker = AIFilePicker::create();
		filepicker->open(name);
		filepicker->run(boost::bind(&saveImpl, daesaver, filepicker));
	}

	void onPatialExportConfirm(const LLSD& notification, const LLSD& response, DAESaver* daesaver, std::string name)
	{
		if (LLNotificationsUtil::getSelectedOption(notification, response) == 0) // 0 - Proceed, first choice
		{
			filePicker(daesaver, name);
		}
		else
		{
			delete daesaver;
		}
	}

	bool canExportNode(const LLSelectNode* node)
	{
		if (const LLPermissions* perms = node->mPermissions)
		{
			if (gAgentID == perms->getCreator() || (LFSimFeatureHandler::instance().simSupportsExport() && gAgentID == perms->getOwner() && perms->getMaskEveryone() & PERM_EXPORT))
			{
				return true;
			}
		}
		return false;
	}

	void saveSelectedObject()
	{
		static const std::string file_ext = ".dae";

		if (LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection())
		{
			DAESaver* daesaver = new DAESaver; // deleted in callback
			daesaver->mOffset = -selection->getFirstRootObject()->getRenderPosition();
			S32 total = 0;
			S32 included = 0;

			for (LLObjectSelection::iterator iter = selection->begin(); iter != selection->end(); ++iter)
			{
				total++;
				LLSelectNode* node = *iter;
				if (!canExportNode(node) || !node->getObject()->getVolume()) continue;
				included++;
				daesaver->Add(node->getObject(), node->mName);
			}

			if (daesaver->mObjects.empty())
			{
				LLNotificationsUtil::add("ExportFailed");
				delete daesaver;
				return;
			}

			if (total != included)
			{
				LLSD args;
				args["TOTAL"] = total;
				args["FAILED"] = total - included;
				LLNotificationsUtil::add("WavefrontExportPartial", args, LLSD(), boost::bind(&onPatialExportConfirm, _1, _2, daesaver, selection->getFirstNode()->mName.c_str() + file_ext));
				return;
			}

			filePicker(daesaver, selection->getFirstNode()->mName.c_str() + file_ext);
		}
		return;
	}

	class DAESaveSelectedObjects : public view_listener_t
	{
		bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
		{
			saveSelectedObject();
			return true;
		}
	};
}


void DAESaver::Add(const LLViewerObject* prim, const std::string name)
{
	mObjects.push_back(std::pair<LLViewerObject*,std::string>((LLViewerObject*)prim, name));
}

void DAESaver::DAESaveAccessor(domAccessor* acc, int numValues, std::string params)
{
	acc->setCount(numValues);
	acc->setStride(params.size());

	for (std::string::iterator p_iter = params.begin(); p_iter != params.end(); ++p_iter)
	{
		domElement* pX = acc->add("param");
		pX->setAttribute("name", llformat("%c", *p_iter).c_str());
		pX->setAttribute("type", "float");
	}
}

class v4adapt
{
private:
	LLStrider<LLVector4a> mV4aStrider;
public:
	v4adapt(LLVector4a* vp){ mV4aStrider = vp; }
	inline LLVector3 operator[] (const unsigned int i)
	{
		return LLVector3((F32*)&mV4aStrider[i]);
	}
};

bool DAESaver::saveDAE(std::string filename)
{
	DAE dae;
	// First set the filename to save
	daeElement* root = dae.add(filename);

	// Obligatory elements in header
	daeElement* asset = root->add("asset");
	// Get ISO format time
	std::string date = boost::posix_time::to_iso_extended_string(boost::posix_time::second_clock::local_time());
	daeElement* created = asset->add("created");
	created->setCharData(date);
	daeElement* modified = asset->add("modified");
	modified->setCharData(date);
	daeElement* unit = asset->add("unit");
	unit->setAttribute("name", "meter");
	unit->setAttribute("value", "1");
	daeElement* up_axis = asset->add("up_axis");
	up_axis->setCharData("Z_UP");

	// File creator
	daeElement* contributor = asset->add("contributor");
	contributor->add("author")->setCharData("Singularity User");
	contributor->add("authoring_tool")->setCharData("Singularity Viewer Collada Export");

	daeElement* geomLib = root->add("library_geometries");
	daeElement* scene = root->add("library_visual_scenes visual_scene");
	scene->setAttribute("id", "Scene");
	scene->setAttribute("name", "Scene");
	S32 prim_nr = 0;

	for (obj_info_t::iterator obj_iter = mObjects.begin(); obj_iter != mObjects.end(); ++obj_iter)
	{
		LLViewerObject* obj = obj_iter->first;
		S32 total_num_vertices = 0;

		std::string name = "";
		if (name.empty()) name = llformat("prim%d", prim_nr++);

		const char* geomID = name.c_str();

		daeElement* geom = geomLib->add("geometry");
		geom->setAttribute("id", llformat("%s-%s", geomID, "mesh").c_str());
		daeElement* mesh = geom->add("mesh");

		daeElement* positionsSource = mesh->add("source");
		positionsSource->setAttribute("id", llformat("%s-%s", geomID, "positions").c_str());
		daeElement* positionsArray = positionsSource->add("float_array");

		daeElement* uvSource = mesh->add("source");
		uvSource->setAttribute("id", llformat("%s-%s", geomID, "map-0").c_str());
		daeElement* uvArray = uvSource->add("float_array");

		daeElement* normalsSource = mesh->add("source");
		normalsSource->setAttribute("id", llformat("%s-%s", geomID, "normals").c_str());
		daeElement* normalsArray = normalsSource->add("float_array");

		S32 num_faces = obj->getVolume()->getNumVolumeFaces();
		for (S32 face_num = 0; face_num < num_faces; face_num++)
		{
			const LLVolumeFace* face = (LLVolumeFace*)&obj->getVolume()->getVolumeFace(face_num);
			total_num_vertices += face->mNumVertices;

			// Positions
			v4adapt verts(face->mPositions);
			for (S32 i=0; i < face->mNumVertices; i++)
			{
				((domFloat_array*)positionsArray)->getValue().append3(verts[i].mV[VX], verts[i].mV[VY], verts[i].mV[VZ]);
			}

			// UV map
			for (S32 i=0; i < face->mNumVertices; i++)
			{
				((domFloat_array*)uvArray)->getValue().append2(face->mTexCoords[i].mV[VX], face->mTexCoords[i].mV[VY]);
			}


			// Normals
			v4adapt norms(face->mNormals);
			for (S32 i=0; i < face->mNumVertices; i++)
			{
				const LLVector3 n = norms[i];
				((domFloat_array*)normalsArray)->getValue().append3(n[0], n[1], n[2]);

			}
		}

		// Save positions
		positionsArray->setAttribute("id", llformat("%s-%s", geomID, "positions-array").c_str());
		positionsArray->setAttribute("count", llformat("%d", total_num_vertices * 3).c_str());
		domAccessor* positionsAcc = daeSafeCast<domAccessor>(positionsSource->add("technique_common accessor"));
		positionsAcc->setSource(llformat("#%s-%s", geomID, "positions-array").c_str());
		this->DAESaveAccessor(positionsAcc, total_num_vertices, "XYZ");

		// Save UV map
		uvArray->setAttribute("id", llformat("%s-%s", geomID, "map-0-array").c_str());
		uvArray->setAttribute("count", llformat("%d", total_num_vertices * 2).c_str());
		domAccessor* uvAcc = daeSafeCast<domAccessor>(uvSource->add("technique_common accessor"));
		uvAcc->setSource(llformat("#%s-%s", geomID, "map-0-array").c_str());
		this->DAESaveAccessor(uvAcc, total_num_vertices, "ST");

		// Save normals
		normalsArray->setAttribute("id", llformat("%s-%s", geomID, "normals-array").c_str());
		normalsArray->setAttribute("count", llformat("%d", total_num_vertices * 3).c_str());
		domAccessor* normalsAcc = daeSafeCast<domAccessor>(normalsSource->add("technique_common accessor"));
		normalsAcc->setSource(llformat("#%s-%s", geomID, "normals-array").c_str());
		DAESaveAccessor(normalsAcc, total_num_vertices, "XYZ");

		// Add the <vertices> element
		{
			daeElement*	verticesNode = mesh->add("vertices");
			verticesNode->setAttribute("id", llformat("%s-%s", geomID, "vertices").c_str());
			daeElement* verticesInput = verticesNode->add("input");
			verticesInput->setAttribute("semantic", "POSITION");
			verticesInput->setAttribute("source", llformat("#%s-%s", geomID, "positions").c_str());
		}

		// Add triangles
		domPolylist* polylist = daeSafeCast<domPolylist>(mesh->add("polylist"));
		polylist->setMaterial("mtl");

		// Vertices semantic
		{
			domInputLocalOffset* input = daeSafeCast<domInputLocalOffset>(polylist->add("input"));
			input->setSemantic("VERTEX");
			input->setOffset(0);
			input->setSource(llformat("#%s-%s", geomID, "vertices").c_str());
		}

		// Normals semantic
		{
			domInputLocalOffset* input = daeSafeCast<domInputLocalOffset>(polylist->add("input"));
			input->setSemantic("NORMAL");
			input->setOffset(0);
			input->setSource(llformat("#%s-%s", geomID, "normals").c_str());
		}

		// UV semantic
		{
			domInputLocalOffset* input = daeSafeCast<domInputLocalOffset>(polylist->add("input"));
			input->setSemantic("TEXCOORD");
			input->setOffset(0);
			input->setSource(llformat("#%s-%s", geomID, "map-0").c_str());
		}

		// Save indices
		domP* p = daeSafeCast<domP>(polylist->add("p"));
		domPolylist::domVcount *vcount = daeSafeCast<domPolylist::domVcount>(polylist->add("vcount"));
		S32 index_offset = 0;
		S32 num_tris = 0;
		for (S32 face_num = 0; face_num < num_faces; face_num++)
		{
			const LLVolumeFace* face = (LLVolumeFace*)&obj->getVolume()->getVolumeFace(face_num);
			for (S32 i = 0; i < face->mNumIndices; i++)
			{
				U16 index = index_offset + face->mIndices[i];
				(p->getValue()).append(index);
				if (i % 3 == 0)
				{
					(vcount->getValue()).append(3);
					num_tris++;
				}
			}
			index_offset += face->mNumVertices;
		}
		polylist->setCount(num_tris);

		// Add scene
		daeElement* node = scene->add("node");
		node->setAttribute("type", "NODE");
		node->setAttribute("id", geomID);
		node->setAttribute("name", geomID);

		// Set tranform matrix (object position, rotation and scale)
		domMatrix* matrix = (domMatrix*)node->add("matrix");
		LLXform srt;
		srt.setScale(obj->getScale());
		srt.setPosition(obj->getRenderPosition() + mOffset);
		srt.setRotation(obj->getRenderRotation());
		LLMatrix4 m4;
		srt.getLocalMat4(m4);
		for (int i=0; i<4; i++)
			for (int j=0; j<4; j++)
				(matrix->getValue()).append(m4.mMatrix[j][i]);

		daeElement* nodeGeometry = node->add("instance_geometry");
		nodeGeometry->setAttribute("url", llformat("#%s-%s", geomID, "mesh").c_str());
	}
	root->add("scene instance_visual_scene")->setAttribute("url", "#Scene");

	return dae.writeAll();
}

DAESaver::DAESaver()
{}

void addMenu(view_listener_t* menu, const std::string& name);
void add_dae_listeners() // Called in llviewermenu with other addMenu calls, function linked against
{
	addMenu(new DAEExportUtil::DAESaveSelectedObjects(), "Object.SaveAsDAE");
}
