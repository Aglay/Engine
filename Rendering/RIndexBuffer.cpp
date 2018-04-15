#include "Precompile.h"
#include "Rendering/RIndexBuffer.h"

using namespace Helium;

/// Destructor.
RIndexBuffer::~RIndexBuffer()
{
}

/// @fn void* RIndexBuffer::Map( ERendererBufferMapHint hint )
/// Map the buffer contents to a CPU-accessible memory address.
///
/// This allows for updating the contents of an existing buffer.  When the application is done writing to the
/// buffer, it must call Unmap() to release the mapped address.
///
/// Depending on the buffer type and the target platform, a mapped buffer is likely not to support reading by the
/// CPU.  Doing so may cause a performance hit or even crashes.
///
/// @param[in] hint  Hint to pass to the renderer as to how existing data will be treated.
///
/// @return  Mapped address if this buffer was mapped successfully, null if not.
///
/// @see Unmap()

/// @fn void RIndexBuffer::Unmap()
/// Unmap a buffer previously mapped to a CPU-accessible memory address using Map().
///
/// @see Map()
