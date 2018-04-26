#pragma once

#include "Platform/Assert.h"
#include "Foundation/Functions.h"
#include "EngineJobs/EngineJobs.h"
#include "EngineJobs/EngineJobsTypes.h"

namespace Helium
{

/// Parallel array quick sort.
template< typename T, typename CompareFunction = Less< T > >
class SortJob : Helium::NonCopyable
{
public:
    class Parameters
    {
    public:
        /// [inout] Pointer to the first element to sort.
        T* pBase;
        /// [in] Number of elements to sort.
        size_t count;
        /// [in] Function object for checking whether the first element should be sorted before the second element.
        CompareFunction compare;
        /// [in] Sub-division size at which to run the remainder of the sort within a single job.
        size_t singleJobCount;

        /// @name Construction/Destruction
        //@{
        inline Parameters();
        //@}
    };

    /// @name Construction/Destruction
    //@{
    inline SortJob();
    inline ~SortJob();
    //@}

    /// @name Parameters
    //@{
    inline Parameters& GetParameters();
    inline const Parameters& GetParameters() const;
    inline void SetParameters( const Parameters& rParameters );
    //@}

    /// @name Job Execution
    //@{
    void Run();
    inline static void RunCallback( void* pJob );
    //@}

private:
    Parameters m_parameters;
};

}  // namespace Helium

#include "EngineJobs/EngineJobsInterface.inl"
#include "EngineJobs/SortJob.inl"
