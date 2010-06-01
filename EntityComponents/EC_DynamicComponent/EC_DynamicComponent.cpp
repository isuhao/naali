// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "EC_DynamicComponent.h"
#include "ModuleInterface.h"
#include "ModuleManager.h"
#include "Entity.h"
//#include "SceneManager.h" //to emit ComponentInitialized signal via scene

#include <qvariant.h>

#define LogError(msg) Poco::Logger::get("EC_DynamicComponent").error(std::string("Error: ") + msg);
#define LogInfo(msg) Poco::Logger::get("EC_DynamicComponent").information(msg);

EC_DynamicComponent::EC_DynamicComponent(Foundation::ModuleInterface *module) :
  Foundation::ComponentInterface(module->GetFramework()),
    x_(this, "x", 0)
{
  //QObject::connect(this, SIGNAL(OnChanged()), this, SLOT(UpdateWidgetAndCanvas()));
  //emit On
    LogInfo("Constructor");
    /* the parent entity is not set here yet, so could not do this.
       but discovered that rexlogic already emits ComponentAdded upon loading, so using that now
    Scene::Entity* entity = GetParentEntity();
    if (entity)
    {
        LogInfo("Parent Entity");

        Scene::SceneManager* scene = entity->GetScene();
        if (scene)
        {
            LogInfo("Scene");
            scene->EmitComponentInitialized(this);
            LogInfo("Emitted ComponentInitialized");
        }
        }*/
}

EC_DynamicComponent::~EC_DynamicComponent()
{
}

void EC_DynamicComponent::AddAttribute()
{
    LogInfo("AddAttribute");
}

QVariant EC_DynamicComponent::GetAttribute()
{
    LogInfo("GetAttribute");
    QVariant v(x_.Get());
    return v;
}

void EC_DynamicComponent::SetAttribute(float new_x)
{
    LogInfo("SetAttribute");
    x_.Set(new_x, Foundation::ComponentInterface::Local);
    ComponentChanged(Foundation::ComponentInterface::Local);
}

uint EC_DynamicComponent::GetParentEntityId()
{
    return (uint)GetParentEntity()->GetId();
}