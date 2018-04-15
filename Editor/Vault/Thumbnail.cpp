#include "Precompile.h"
#include "Thumbnail.h"
#include "EditorScene/DeviceManager.h"

using namespace Helium;
using namespace Helium::Editor;

Thumbnail::Thumbnail( DeviceManager* d3dManager )
: m_DeviceManager( d3dManager )
#ifdef VIEWPORT_REFACTOR
, m_Texture( NULL )
#endif
, m_IsFromIcon( false )
{
}

#ifdef VIEWPORT_REFACTOR

Thumbnail::Thumbnail( DeviceManager* d3dManager, IDirect3DTexture9* texture )
: m_DeviceManager( d3dManager )
, m_Texture( texture )
, m_IsFromIcon( false )
{
}

#endif

Thumbnail::~Thumbnail()
{
#ifdef VIEWPORT_REFACTOR
  if ( m_Texture )
  {
    m_Texture->Release();
  }
#endif
}

#ifdef VIEWPORT_REFACTOR

bool Thumbnail::FromIcon( HICON icon )
{
  wxIcon temp;
  temp.SetHICON( icon );

  if ( temp.IsOk() )
  {
    wxBitmap bitmap ( temp );

    if ( bitmap.IsOk() )
    {
      wxImage image = bitmap.ConvertToImage();

      if ( image.IsOk() )
      {
        // create texture
        if ( SUCCEEDED( m_DeviceManager->GetD3DDevice()->CreateTexture(image.GetWidth(), image.GetHeight(), 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_Texture, NULL) ) )
        {
          // lock texture
          D3DLOCKED_RECT rect;
          if( SUCCEEDED( m_Texture->LockRect( 0, &rect, NULL, D3DLOCK_NOSYSLOCK ) ) )
          {
            // copy bitmap data to texture
            DWORD* dest = (DWORD*)rect.pBits;

            // copy the pixels
            for ( int y = 0; y < image.GetHeight(); y++ )
            {
              for( int x = 0 ; x < image.GetWidth() ; x++ )
              {
                dest[ x ] = image.HasAlpha() ? image.GetAlpha( x, y ) : 0xFF << 24 | image.GetRed( x, y ) << 16 | image.GetGreen( x, y ) << 8 | image.GetBlue( x, y );
              }

              // skip to next row of pixels
	            dest += rect.Pitch / 4;
            }

            m_Texture->UnlockRect( 0 );
          }
        }
      }
    }
  }

  m_IsFromIcon = m_Texture != NULL;

  return m_IsFromIcon;
}

#endif
