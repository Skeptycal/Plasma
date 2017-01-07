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
#include "HeadSpin.h"
#include "hsWindows.h"

#include <d3d9.h>
#include <d3dx9core.h>

#include "plDXPixelShader.h"

#include "plSurface/plShader.h"

#include "plDXPipeline.h"


plDXPixelShader::plDXPixelShader(plShader* owner)
:   plDXShader(owner), fHandle(nil)
{
}

plDXPixelShader::~plDXPixelShader()
{
    Release();
}

void plDXPixelShader::Release()
{
    ReleaseObject(fHandle);
    fHandle = nil;
    fPipe = nil;

    ISetError(nil);
}

bool plDXPixelShader::VerifyFormat(uint8_t format) const
{
    return (fOwner->GetInputFormat() & format) == fOwner->GetInputFormat();
}

IDirect3DPixelShader9 *plDXPixelShader::GetShader(plDXPipeline* pipe)
{
    HRESULT hr = S_OK;
    if ( !fHandle )
    {
        if( FAILED(hr = ICreate(pipe)) )
            return nil;
    }

    if( FAILED(hr = ISetConstants(pipe)) )
        return nil;

    return fHandle;
}

HRESULT plDXPixelShader::ICreate(plDXPipeline* pipe)
{
    fHandle = nil; // in case something goes wrong.
    fPipe = nil;
    ISetError(nil);

#ifdef HS_DEBUGGING
    DWORD   flags = D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION;
#else // HS_DEBUGGING
    DWORD   flags = 0;
#endif // HS_DEBUGGING

    DWORD* shaderCodes = nil;

    HRESULT hr = S_OK;
    if( plShaderTable::LoadFromFile() || !fOwner->GetDecl()->GetCodes() )
    {
        if( fOwner->GetDecl()->GetFileName() )
        {
            LPD3DXBUFFER compiledShader = nil;
            LPD3DXBUFFER compilationErrors = nil;

            hr = D3DXAssembleShaderFromFile(
                            fOwner->GetDecl()->GetFileName(),
                            NULL, NULL, flags,
                            &compiledShader,
                            &compilationErrors);

            if( FAILED(hr) )
            {
                return IOnError(hr, compilationErrors
                        ? reinterpret_cast<const char *>(compilationErrors->GetBufferPointer())
                        : "File not found");
            }

            shaderCodes = (DWORD*)(compiledShader->GetBufferPointer());
        }
    }
    if( !shaderCodes )
    {
        shaderCodes = (DWORD*)(fOwner->GetDecl()->GetCodes());
    }
    if( !shaderCodes )
        return IOnError(-1, "No file and no compiled codes");

    hr = pipe->GetD3DDevice()->CreatePixelShader(shaderCodes, &fHandle);
    if( FAILED(hr) )
    {
        return IOnError(hr, "Error on CreatePixelShader");
    }

    hsAssert(fHandle, "No error, but no pixel shader handle. Grrrr.");

    fPipe = pipe;

    return S_OK;
}

HRESULT plDXPixelShader::ISetConstants(plDXPipeline* pipe)
{
    hsAssert(fHandle, "Pixel shader called to set constants without initialization");
    if( fOwner->GetNumConsts() )
    {
        HRESULT hr = pipe->GetD3DDevice()->SetPixelShaderConstantF(0,
                        (float*)fOwner->GetConstBasePtr(),
                        fOwner->GetNumConsts());
        if( FAILED(hr) )
            return IOnError(hr, "Error setting constants");
    }

    return S_OK;
}