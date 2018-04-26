
#include "Precompile.h"
#include "Bullet/Bullet.h"
#include "Bullet/BulletEngine.h"
#include "Bullet/BulletBodyComponent.h"

#include "Reflect/TranslatorDeduction.h"

using namespace Helium;

HELIUM_IMPLEMENT_ASSET( Helium::BulletSystemComponent, Bullet, 0 )

void Helium::BulletSystemComponent::Initialize()
{
	ms_Instance = this;
	HELIUM_ASSERT( !m_BodyFlags || m_BodyFlags->GetFlagCount() < BulletBodyComponent::MAX_BULLET_BODY_FLAGS );
}

void Helium::BulletSystemComponent::Cleanup()
{
}

void Helium::BulletSystemComponent::Destroy()
{
	ms_Instance = NULL;
}

void BulletSystemComponent::PopulateMetaType( Reflect::MetaStruct& comp )
{
	comp.AddField( &BulletSystemComponent::m_BodyFlags, "m_BodyFlags" );
}

BulletSystemComponent *BulletSystemComponent::ms_Instance = NULL;