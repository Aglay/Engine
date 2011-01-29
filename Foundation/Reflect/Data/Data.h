#pragma once

#include <sstream>
#include <iostream>

#include "Foundation/Automation/Event.h"
#include "Foundation/Reflect/Object.h"
#include "Foundation/Reflect/Archive.h"
#include "Foundation/SmartBuffer/BasicBuffer.h"

namespace Helium
{
    namespace Reflect
    {
        namespace DataFlags
        {
            enum DataFlag
            {
                Shallow   = 1 << 0,
            };
        }

        typedef DataFlags::DataFlag DataFlag;

        //
        // A pointer to some typed data (owned by the object itself or someone else
        //

        template<class T>
        class DataPointer
        {
        public:
            DataPointer();
            ~DataPointer();

            void Allocate() const;
            void Deallocate() const;

            void Connect(void* pointer);
            void Disconnect();

            const T* operator->() const;
            T* operator->();

            operator const T*() const;
            operator T*();

        private:
            mutable T*      m_Target;
            mutable bool    m_Owned;
        };

        //
        // Like DataPointer, but typeless
        //

        class VoidDataPointer
        {
        public:
            VoidDataPointer();
            ~VoidDataPointer();

            void Allocate( uint32_t size ) const;
            void Deallocate() const;

            void Connect(void* pointer, uint32_t size);
            void Disconnect();

            const void* Get(uint32_t size) const;
            void* Get(uint32_t size);

        private:
            mutable void*       m_Target;
            mutable bool        m_Owned;
#ifdef REFLECT_CHECK_MEMORY
            mutable uint32_t    m_Size;
#endif
        };
        
        //
        // A Data is an Object that knows how to read/write data
        //  from any kind of support Archive type (XML and Binary), given
        //  an address in memory to serialize/deserialize data to/from
        //

        class FOUNDATION_API Data : public Object
        {
        protected:
            // the instance we are processing, if any
            void* m_Instance;

            // the field we are processing, if any
            const Field* m_Field;

        public:
            REFLECT_DECLARE_ABSTRACT( Data, Object );

            // instance init and cleanup
            Data();
            virtual ~Data();

            // static init and cleanup
            static void Initialize();
            static void Cleanup();

            //
            // Connection
            //

            // set the address to interface with
            virtual void ConnectData(void* data) = 0;

            // connect to a field of an object
            void ConnectField(void* instance, const Field* field, uintptr_t offsetInField = 0);

            // reset all pointers
            void Disconnect();

            //
            // Specializations
            //

            template<class T>
            static inline T* GetData(Data*);

            //
            // Creation templates
            //

            template <class T>
            static DataPtr Create();

            template <class T>
            static DataPtr Create(const T& value);

            template <class T>
            static DataPtr Bind(T& value, void* instance, const Field* field);

            //
            // Value templates
            //

            template <typename T>
            static bool GetValue(Data* ser, T& value);

            template <typename T>
            static bool SetValue(Data* ser, T value, bool raiseEvents = true);

            //
            // Data Management
            //

            // check to see if a cast is supported
            static bool CastSupported(const Class* srcType, const Class* destType);

            // convert value data from one data to another
            static bool CastValue(Data* src, Data* dest, uint32_t flags = 0);

            // copies value data from one data to another
            virtual bool Set(Data* src, uint32_t flags = 0) = 0;

            // assign
            inline Data& operator=(Data* rhs);
            inline Data& operator=(Data& rhs);

            // equality
            inline bool operator==(Data* rhs);
            inline bool operator== (Data& rhs);

            // inequality
            inline bool operator!=(Data* rhs);
            inline bool operator!=(Data& rhs);

            //
            // Serialization
            //

            // is this data worth serializing? (perhaps its an empty container?)
            virtual bool ShouldSerialize();

            // data serialization (extract to smart buffer)
            virtual void Serialize(const Helium::BasicBufferPtr& buffer, const tchar_t* debugStr) const;

            // data serialization (extract to archive)
            virtual void Serialize(Archive& archive) = 0;

            // data deserialization (insert from archive)
            virtual void Deserialize(Archive& archive) = 0;

            // text serialization (extract to text stream)
            virtual tostream& operator>>(tostream& stream) const;

            // text deserialization (insert from text stream)
            virtual tistream& operator<<(tistream& stream);

            //
            // Visitor
            //

            virtual void Accept(Visitor& visitor) HELIUM_OVERRIDE;
        };

        //
        // These are the global entry points for our virtual insert an extract functions
        //

        inline tostream& operator<<(tostream& stream, const Data& s);
        inline tistream& operator>>(tistream& stream, Data& s);

        //
        // Data class deduction function
        //

        template<class T>
        static const Class* GetDataClass();
    }
}

#include "Foundation/Reflect/Data/Data.inl"