
#pragma once

#include "Framework/Framework.h"

#include "Foundation/DynamicArray.h"
#include "Foundation/ReferenceCounting.h"

#define HELIUM_DECLARE_TASK(__Type)                         \
		__Type();                                           \
		static __Type m_This; 


#define HELIUM_DEFINE_TASK(__Type, __Function, __TickType)  \
	__Type __Type::m_This;                                  \
	__Type::__Type()                                        \
		: TaskDefinition(m_This, __Function, #__Type) \
	{                                                       \
		m_Contract.SetTickType( __TickType );               \
	}

// Abstract tasks are used when you want a conceptual thing like "render" to be a dependency that other tasks
// can say they go before, after, or fulfill. This allows us to generally define a few high-level stages and 
// let client code non-intrusively hook their logic to run within these stages, or even
// define their own concepts for other code to non-obtrusively hook. This also lets us decouple tasks from each
// other (i.e. rather than AI requiring OIS input to be pulled, it can require the abstract concept of "ReceiveInput",
// which an OIS input grabbing task might fulfill, ensuring that the AI need not "know" about OIS)
#define HELIUM_DEFINE_ABSTRACT_TASK(__Type)                 \
	__Type __Type::m_This;                                  \
	__Type::__Type()                                        \
		: TaskDefinition(m_This, 0, #__Type)          \
	{                                                       \
															\
	}


namespace Helium
{	
	struct TaskDefinition;

	namespace OrderRequirementTypes
	{
		enum OrderRequirementType
		{
			Before,
			After, 
		};
	}
	typedef OrderRequirementTypes::OrderRequirementType OrderRequirementType;

	namespace TickTypes
	{
		enum TickType
		{
			Render              = 1<<0, // If it's required for rendering
			Gameplay            = 1<<1, // If it's required for gameplay (i.e. dedicated server)
			Client              = 1<<2, // If it's required to let someone play locally (i.e. listen server or single player)
			EditTime            = 1<<3, // If it's required for editor to function

			Never               = 0,
			Always              = 0xFFFFFFFF,

			HeadlessGame        = Gameplay,
			RenderingGame       = Render | Gameplay | Client,
			Editor              = Render | EditTime
		};
	}
	typedef TickTypes::TickType TickType;

	struct OrderRequirement
	{
		TaskDefinition *m_Dependency;
		OrderRequirementType m_Type;
	};

	// Defines what the task expects and what it provides
	struct TaskContract
	{
		TaskContract()
			: m_TickType( TickTypes::Never )
		{

		}

		// Task T must execute before this task
		template <class T>
		void ExecuteBefore()
		{
			ExecuteBefore(T::m_This);
		}

		// Task T must execute after this task
		template <class T>
		void ExecuteAfter()
		{
			ExecuteAfter(T::m_This);
		}
		
		void ExecuteBefore(TaskDefinition &rDependency)
		{
			OrderRequirement *requirement = m_OrderRequirements.New();
			requirement->m_Dependency = &rDependency;
			requirement->m_Type = OrderRequirementTypes::Before;
		}
				
		void ExecuteAfter(TaskDefinition &rDependency)
		{
			OrderRequirement *requirement = m_OrderRequirements.New();
			requirement->m_Dependency = &rDependency;
			requirement->m_Type = OrderRequirementTypes::After;
		}
		
		template <class T>
		void ExecutesWithin()
		{
			ExecutesWithin(T::m_This);
		}
		
		void ExecutesWithin(const TaskDefinition &rDependency)
		{
			m_ContributedDependencies.Push(&rDependency);
		}

		void SetTickType(TickType tickType)
		{
			m_TickType = tickType;
		}

		// Every requirement to be before or after another dependency goes here
		DynamicArray<OrderRequirement> m_OrderRequirements;

		// All dependencies we contribute to fulfilling
		DynamicArray<const TaskDefinition *> m_ContributedDependencies;

		TickType m_TickType;
	};

	class World;
	typedef Helium::StrongPtr< World > WorldPtr;
	typedef void (*TaskFunc)( DynamicArray< WorldPtr > & );

	struct HELIUM_FRAMEWORK_API TaskDefinition
	{
		TaskDefinition(const TaskDefinition &rDependency, TaskFunc pFunc, const char *pName)
			: m_DependencyReverseLookup(rDependency)
			, m_Func(pFunc)
			, m_Next(s_FirstTaskDefinition)
#if HELIUM_TOOLS
			, m_Name(pName)
#endif
		{
			m_Contract.ExecutesWithin(rDependency);

			s_FirstTaskDefinition = this;
		}
		
		// Scaffolding to allow child classes to define their contract
		virtual void DefineContract(TaskContract &) = 0;
		void DoDefineContract()
		{
			DefineContract(m_Contract);
		}
		
		// We build this list of tasks that must execute before us in TaskScheduler::CalculateSchedule()
		DynamicArray<const TaskDefinition *> m_RequiredTasks;

#if HELIUM_TOOLS
		// Task name useful for debug purposes
		const char *m_Name;
#endif

		// Our contract to be filled out by subclass
		TaskContract m_Contract;

		// The callback that will execute this task
		TaskFunc m_Func;
		
		const TaskDefinition &m_DependencyReverseLookup;

		// Support for maintaining a linked list of all created task definitions (only one per type should ever exist)
		TaskDefinition *m_Next;
		static TaskDefinition *s_FirstTaskDefinition;
	};
	typedef DynamicArray<const TaskDefinition *> A_TaskDefinitionPtr;

	struct TaskSchedule
	{
		A_TaskDefinitionPtr m_ScheduleInfo;
		DynamicArray<TaskFunc> m_ScheduleFunc; // Compact version of our schedule
	};

	class HELIUM_FRAMEWORK_API TaskScheduler
	{
	public:
		static bool CalculateSchedule( uint32_t tickType, TaskSchedule &schedule );
		static void ExecuteSchedule( const TaskSchedule &schedule, DynamicArray< WorldPtr > &rWorlds );

		static void ResetContracts();

		static bool m_ContractsDefined;
	};

	namespace StandardDependencies
	{
		struct HELIUM_FRAMEWORK_API ReceiveInput : public TaskDefinition
		{
			HELIUM_DECLARE_TASK(ReceiveInput);
			virtual void DefineContract(TaskContract &r);
		};

		struct HELIUM_FRAMEWORK_API PrePhysicsGameplay : public TaskDefinition
		{
			HELIUM_DECLARE_TASK(PrePhysicsGameplay);
			virtual void DefineContract(TaskContract &r);
		};
				
		struct HELIUM_FRAMEWORK_API ProcessPhysics : public TaskDefinition
		{
			HELIUM_DECLARE_TASK(ProcessPhysics);
			virtual void DefineContract(TaskContract &r);
		};

		struct HELIUM_FRAMEWORK_API PostPhysicsGameplay : public TaskDefinition
		{
			HELIUM_DECLARE_TASK(PostPhysicsGameplay);
			virtual void DefineContract(TaskContract &r);
		};
			  
		struct HELIUM_FRAMEWORK_API Render : public TaskDefinition
		{
			HELIUM_DECLARE_TASK(Render);
			virtual void DefineContract(TaskContract &r);
		};

		struct HELIUM_FRAMEWORK_API PostRender : public TaskDefinition
		{
			HELIUM_DECLARE_TASK(PostRender);
			virtual void DefineContract(TaskContract &r);
		};
	};

	template < void (*Fn)(World *) >
	void ForEachWorld(DynamicArray< WorldPtr > &rWorlds)
	{
		for (DynamicArray< WorldPtr >::Iterator iter = rWorlds.Begin();
			iter != rWorlds.End(); ++iter)
		{
			Fn( iter->Get() );
		}
	}
}
