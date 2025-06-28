/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "Component.hpp"
#include "Convert.hpp"

#include <wx/log.h>

Component::Component(irr::scene::ESCENE_NODE_ANIMATOR_TYPE type, 
    irr::io::IAttributes* attributes)
    : m_Type(type), m_Attributes(attributes)
{
#ifdef _DEBUG
	setDebugName("Component");
#endif

    if (m_Attributes)
        m_Attributes->grab();
}

Component::~Component(void)
{
    if (m_Attributes)
        m_Attributes->drop();
}

void Component::animateNode(irr::scene::ISceneNode* node, irr::u32 timeMs)
{
    // do nothing
}

irr::scene::ISceneNodeAnimator* Component::createClone(irr::scene::ISceneNode* node,
    irr::scene::ISceneManager* newManager)
{
    // if ()
    return nullptr;
}

irr::scene::ESCENE_NODE_ANIMATOR_TYPE Component::getType(void) const
{
    return m_Type;
}

bool Component::hasFinished(void) const
{
    return true;
}

bool Component::isEventReceiverEnabled(void) const
{
    return false;
}

bool Component::OnEvent(const irr::SEvent& event)
{
    return false;
}

void Component::deserializeAttributes(irr::io::IAttributes *in,
    irr::io::SAttributeReadWriteOptions *options)
{
    // read the attributes
    for (irr::u32 i = 0; i < in->getAttributeCount(); ++i)
    {
        const irr::c8* name = in->getAttributeName(i);
        switch (in->getAttributeType(i))
        {
        case irr::io::EAT_INT:
            m_Attributes->setAttribute(name, in->getAttributeAsInt(i));
            break;
        case irr::io::EAT_FLOAT:
            m_Attributes->setAttribute(name, in->getAttributeAsFloat(i));
            break;
        case irr::io::EAT_STRING:
            m_Attributes->setAttribute(name, in->getAttributeAsString(i).c_str());
            break;
        case irr::io::EAT_BOOL:
            m_Attributes->setAttribute(name, in->getAttributeAsBool(i));
            break;
        case irr::io::EAT_COLOR:
            m_Attributes->setAttribute(name, in->getAttributeAsColor(i));
            break;
        case irr::io::EAT_COLORF:
            m_Attributes->setAttribute(name, in->getAttributeAsColorf(i));
            break;
        case irr::io::EAT_VECTOR3D:
            m_Attributes->setAttribute(name, in->getAttributeAsVector3d(i));
            break;
        case irr::io::EAT_VECTOR2D:
            m_Attributes->setAttribute(name, in->getAttributeAsVector2d(i));
            break;
        }
    }
}

void Component::serializeAttributes(irr::io::IAttributes *out,
    irr::io::SAttributeReadWriteOptions *options) const
{
    // write the attributes
    for (irr::u32 i = 0; i < m_Attributes->getAttributeCount(); ++i)
    {
        const irr::c8* name = m_Attributes->getAttributeName(i);
        switch (m_Attributes->getAttributeType(i))
        {
        case irr::io::EAT_INT:
            out->addInt(name, m_Attributes->getAttributeAsInt(i));
            break;
        case irr::io::EAT_FLOAT:
            out->addFloat(name, m_Attributes->getAttributeAsFloat(i));
            break;
        case irr::io::EAT_STRING:
            out->addString(name, m_Attributes->getAttributeAsString(i).c_str());
            break;
        case irr::io::EAT_BOOL:
            out->addBool(name, m_Attributes->getAttributeAsBool(i));
            break;
        case irr::io::EAT_COLOR:
            out->addColor(name, m_Attributes->getAttributeAsColor(i));
            break;
        case irr::io::EAT_COLORF:
            out->addColorf(name, m_Attributes->getAttributeAsColorf(i));
            break;
        case irr::io::EAT_VECTOR3D:
            out->addVector3d(name, m_Attributes->getAttributeAsVector3d(i));
            break;
        case irr::io::EAT_VECTOR2D:
            out->addVector2d(name, m_Attributes->getAttributeAsVector2d(i));
            break;
        }
    }
}

irr::core::array<ComponentFactory::ComponentType> ComponentFactory::ms_SupportedComponentTypes;
std::map<irr::scene::ESCENE_NODE_ANIMATOR_TYPE, wxXmlDocument> ComponentFactory::ms_ComponentDefinitions;

ComponentFactory::ComponentFactory(irr::scene::ISceneManager* sceneMgr)
    : m_SceneMgr(sceneMgr)
{
#ifdef _DEBUG
	setDebugName("ComponentFactory");
#endif
}

ComponentFactory::~ComponentFactory(void)
{
}

irr::scene::ISceneNodeAnimator* ComponentFactory::createSceneNodeAnimator(irr::scene::ESCENE_NODE_ANIMATOR_TYPE type, 
    irr::scene::ISceneNode *target)
{
    Component* anim = nullptr;

    // find the component type
    for (irr::u32 i = 0; i < ms_SupportedComponentTypes.size(); ++i)
    {
        if (ms_SupportedComponentTypes[i].Type == type)
        {
            irr::io::IAttributes* attributes = m_SceneMgr->getFileSystem()->createEmptyAttributes();
            anim = new Component(ms_SupportedComponentTypes[i].Type, attributes);
            attributes->drop(); // grabbed by the component
            break;
        }
    }

    if (anim)
    {
        // read the component definition, if it exists
        if (ms_ComponentDefinitions.find(type) != ms_ComponentDefinitions.end())
        {
            wxXmlDocument definition = ms_ComponentDefinitions[type];
            wxXmlNode* attributes = definition.GetRoot()->GetChildren();
            while (attributes)
            {
                // each attribute has a single key and value
                wxXmlAttribute* attribute = attributes->GetAttributes();
                wxString key = attribute->GetName();
                wxString value = attribute->GetValue();

                if (attributes->GetName().CompareTo(wxT("int"), wxString::ignoreCase) == 0)
                {
                    anim->m_Attributes->addInt(key.c_str(), valueToInt(value));
                }
                else if (attributes->GetName().CompareTo(wxT("float"), wxString::ignoreCase) == 0)
                {
                    anim->m_Attributes->addFloat(key.c_str(), valueToFloat(value));
                }
                else if (attributes->GetName().CompareTo(wxT("string"), wxString::ignoreCase) == 0)
                {
                    anim->m_Attributes->addString(key.c_str().AsChar(), value.c_str().AsChar());
                }
                else if (attributes->GetName().CompareTo(wxT("bool"), wxString::ignoreCase) == 0)
                {
                    anim->m_Attributes->addBool(key.c_str(), valueToBool(value));
                }
                else if (attributes->GetName().CompareTo(wxT("color"), wxString::ignoreCase) == 0)
                {
                    anim->m_Attributes->addColor(key.c_str(), valueToColor(value));
                }
                else if (attributes->GetName().CompareTo(wxT("vec2"), wxString::ignoreCase) == 0)
                {
                    anim->m_Attributes->addVector2d(key.c_str(), valueToVec2(value));
                }
                else if (attributes->GetName().CompareTo(wxT("vec3"), wxString::ignoreCase) == 0)
                {
                    anim->m_Attributes->addVector3d(key.c_str(), valueToVec3(value));
                }
                else if (attributes->GetName().CompareTo(wxT("texture"), wxString::ignoreCase) == 0)
                {
                    // anim->m_Attributes->addTexture(key.c_str(), value.c_str());
                }

                attributes = attributes->GetNext();
            }
        }

        if (target)
            target->addAnimator(anim);
    }

    return anim;
}

irr::scene::ISceneNodeAnimator* ComponentFactory::createSceneNodeAnimator(const irr::c8* type, 
    irr::scene::ISceneNode *target)
{
    return createSceneNodeAnimator(getTypeFromName(type), target);
}

irr::u32 ComponentFactory::getCreatableSceneNodeAnimatorTypeCount(void) const
{
    return ms_SupportedComponentTypes.size();
}

irr::scene::ESCENE_NODE_ANIMATOR_TYPE ComponentFactory::getCreateableSceneNodeAnimatorType(irr::u32 idx) const
{
	if (idx < ms_SupportedComponentTypes.size())
		return ms_SupportedComponentTypes[idx].Type;

	return irr::scene::ESNAT_UNKNOWN;
}

const irr::c8* ComponentFactory::getCreateableSceneNodeAnimatorTypeName(irr::u32 idx) const
{
	if (idx < ms_SupportedComponentTypes.size())
		return ms_SupportedComponentTypes[idx].TypeName.c_str();

	return nullptr;
}

const irr::c8* ComponentFactory::getCreateableSceneNodeAnimatorTypeName(irr::scene::ESCENE_NODE_ANIMATOR_TYPE type) const
{
	for (irr::u32 i = 0; i < ms_SupportedComponentTypes.size(); ++i)
	{
		if (ms_SupportedComponentTypes[i].Type == type)
			return ms_SupportedComponentTypes[i].TypeName.c_str();
	}

	return nullptr;
}

// FNV1a hash found here: https://create.stephan-brumme.com/fnv-hash/
inline uint32_t fnv1a(uint8_t byte, uint32_t hash)
{
    return (byte ^ hash) * 0x01000193;
}

inline uint32_t fnv1a_hash(const char* text, uint32_t hash)
{
    while (*text)
        hash = fnv1a((uint8_t)*text++, hash);
    return hash;
}

irr::scene::ESCENE_NODE_ANIMATOR_TYPE ComponentFactory::HashComponentName(const wxString& name)
{
    // create 32-bit hash of name
    uint32_t hash = fnv1a_hash(name.c_str().AsChar(), 0x811c9dc5);
    return (irr::scene::ESCENE_NODE_ANIMATOR_TYPE)hash;
}

void ComponentFactory::RegisterComponent(const wxString& name, const wxXmlDocument& definition)
{
    irr::scene::ESCENE_NODE_ANIMATOR_TYPE type = HashComponentName(name);
    for (irr::u32 i = 0; i < ms_SupportedComponentTypes.size(); ++i)
    {
        // if the type already exists, don't add it
        if (ms_SupportedComponentTypes[i].Type == type)
            return;
    }

    // add the new component type
    ms_SupportedComponentTypes.push_back(ComponentType(type, name));
    ms_ComponentDefinitions[type] = definition;
}

irr::scene::ESCENE_NODE_ANIMATOR_TYPE ComponentFactory::getTypeFromName(const irr::c8* name) const
{
    for (irr::u32 i = 0; i < ms_SupportedComponentTypes.size(); ++i)
    {
        if (ms_SupportedComponentTypes[i].TypeName == name)
            return ms_SupportedComponentTypes[i].Type;
    }

    return irr::scene::ESNAT_UNKNOWN;
}
