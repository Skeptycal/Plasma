/*==LICENSE==*

CyanWorlds.com Engine - MMOG client, server and tools
Copyright (C) 2011  Cyan Worlds, Inc.

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

Additional permissions under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or
combining it with any of RAD Game Tools Bink SDK, Autodesk 3ds Max SDK,
NVIDIA PhysX SDK, Microsoft DirectX SDK, OpenSSL library, Independent
JPEG Group JPEG library, Microsoft Windows Media SDK, or Apple QuickTime SDK
(or a modified version of those libraries),
containing parts covered by the terms of the Bink SDK EULA, 3ds Max EULA,
PhysX SDK EULA, DirectX SDK EULA, OpenSSL and SSLeay licenses, IJG
JPEG Library README, Windows Media SDK EULA, or QuickTime SDK EULA, the
licensors of this Program grant you additional
permission to convey the resulting work. Corresponding Source for a
non-source form of such a combination shall include the source code for
the parts of OpenSSL and IJG JPEG Library used as well as that of the covered
work.

You can contact Cyan Worlds, Inc. by email legal@cyan.com
 or by snail mail at:
      Cyan Worlds, Inc.
      14617 N Newport Hwy
      Mead, WA   99021

*==LICENSE==*/
//////////////////////////////////////////////////////////////////////
//
// pyVaultImageNode   - a wrapper class to provide interface to the RelVaultNode
//
//////////////////////////////////////////////////////////////////////

#include <Python.h>
#include "plPipeline.h"
#include "hsResMgr.h"
#pragma hdrstop

#include "pyVaultImageNode.h"
#ifndef BUILDING_PYPLASMA
#   include "pyVault.h"
#endif
#include "pyImage.h"
#include "cyMisc.h"

#include "plGImage/plMipmap.h"
#include "plVault/plVault.h"
#include "pnMessage/plRefMsg.h"
#include "plNetClient/plNetClientMgr.h"


static unsigned s_keyseq;

//============================================================================
static plKey CreateAndRefImageKey (unsigned nodeId, plMipmap * mipmap) {
    ST::string keyName = ST::format("VaultImg_{}_{}", nodeId, s_keyseq++);

    plKey key = hsgResMgr::ResMgr()->NewKey(keyName, mipmap, plLocation::kGlobalFixedLoc);

    hsgResMgr::ResMgr()->AddViaNotify(
        key,
        new plGenRefMsg(
            plNetClientMgr::GetInstance()->GetKey(),
            plRefMsg::kOnCreate,
            0,
            plNetClientMgr::kVaultImage
        ),
        plRefFlags::kActiveRef
    );
    
    return key;
}

// should only be created from C++ side
pyVaultImageNode::pyVaultImageNode(RelVaultNode* nfsNode)
: pyVaultNode(nfsNode)
, fMipmapKey(nil)
, fMipmap(nil)
{
}

//create from the Python side
pyVaultImageNode::pyVaultImageNode(int n)
: pyVaultNode(new RelVaultNode)
, fMipmapKey(nil)
, fMipmap(nil)
{
    fNode->SetNodeType(plVault::kNodeType_Image);
}

pyVaultImageNode::~pyVaultImageNode () {
    if (fMipmap && fMipmapKey)
        fMipmapKey->UnRefObject();
}


//==================================================================
// class RelVaultNode : public plVaultNode
//
void pyVaultImageNode::Image_SetTitle( const char * text )
{
    if (!fNode)
        return;

    VaultImageNode image(fNode);
    image.SetImageTitle(text);
}

void pyVaultImageNode::Image_SetTitleW( const wchar_t* text )
{
    if (!fNode)
        return;

    VaultImageNode image(fNode);
    image.SetImageTitle(ST::string::from_wchar(text));
}

ST::string pyVaultImageNode::Image_GetTitle() const
{
    if (fNode) {
        VaultImageNode image(fNode);
        return image.GetImageTitle();
    }
    return ST::null;
}

PyObject* pyVaultImageNode::Image_GetImage( void )
{
    if (!fNode)
        PYTHON_RETURN_NONE;
        
    if (!fMipmap) {
        VaultImageNode access(fNode);
        if (access.ExtractImage(&fMipmap)) {
            fMipmapKey = fMipmap->GetKey();
            if (!fMipmapKey)
                fMipmapKey = CreateAndRefImageKey(fNode->GetNodeId(), fMipmap);
            else
                fMipmapKey->RefObject();
        }
        else
            PYTHON_RETURN_NONE;
    }
    
    return pyImage::New(fMipmap);   
}

void pyVaultImageNode::Image_SetImage(pyImage& image)
{
    if (!fNode)
        return;

    if (fMipmapKey) {
        fMipmapKey->UnRefObject();
        fMipmapKey = nil;
        fMipmap = nil;
    }

    fMipmap = image.GetImage();
    if (fMipmap) {
        VaultImageNode access(fNode);
        access.StuffImage(fMipmap);

        fMipmapKey = image.GetKey();
        if (!fMipmapKey)
            fMipmapKey = CreateAndRefImageKey(fNode->GetNodeId(), fMipmap);
        else
            fMipmapKey->RefObject();
    }
}

void pyVaultImageNode::SetImageFromBuf( PyObject * pybuf )
{
    if (!fNode)
        return;

    if (fMipmapKey) {
        fMipmapKey->UnRefObject();
        fMipmapKey = nil;
        fMipmap = nil;
    }

    uint8_t * buffer = nil;
    Py_ssize_t bytes;
    PyObject_AsReadBuffer(pybuf, (const void **)&buffer, &bytes);
    if (buffer) {
        VaultImageNode access(fNode);
        access.SetImageData(buffer, bytes);
        access.SetImageType(VaultImageNode::kJPEG);
    }
}

void pyVaultImageNode::SetImageFromScrShot()
{
    if (!fNode)
        return;

    if (fMipmapKey) {
        fMipmapKey->UnRefObject();
        fMipmapKey = nil;
        fMipmap = nil;
    }

    if (cyMisc::GetPipeline()) {
        VaultImageNode access(fNode);
        fMipmap = new plMipmap();
        if (cyMisc::GetPipeline()->CaptureScreen(fMipmap, false, 800, 600)) {
            fMipmapKey = fMipmap->GetKey();
            if (!fMipmapKey)
                fMipmapKey = CreateAndRefImageKey(fNode->GetNodeId(), fMipmap);
            else
                fMipmapKey->RefObject();
            access.StuffImage(fMipmap);
        }
        else {
            access.SetImageData(nil, 0);
            access.SetImageType(VaultImageNode::kNone);
            delete fMipmap;
            fMipmap = nil;
        }
    }
}
