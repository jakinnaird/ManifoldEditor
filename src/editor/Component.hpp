/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include <map>

#include <irrlicht.h>
#include <wx/string.h>
#include <wx/xml/xml.h>

class Component : public irr::scene::ISceneNodeAnimator
{
private:
    irr::scene::ESCENE_NODE_ANIMATOR_TYPE m_Type;

public:
    irr::io::IAttributes* m_Attributes;

public:
    Component(irr::scene::ESCENE_NODE_ANIMATOR_TYPE type, 
        irr::io::IAttributes* attributes);
    virtual ~Component(void);

    void animateNode(irr::scene::ISceneNode* node, irr::u32 timeMs);
    irr::scene::ISceneNodeAnimator* createClone(irr::scene::ISceneNode* node,
        irr::scene::ISceneManager* newManager = 0);
    irr::scene::ESCENE_NODE_ANIMATOR_TYPE getType(void) const;
    
    bool hasFinished(void) const;
    bool isEventReceiverEnabled(void) const;
    bool OnEvent(const irr::SEvent& event);

    void deserializeAttributes(irr::io::IAttributes *in,
        irr::io::SAttributeReadWriteOptions *options=0);
    void serializeAttributes(irr::io::IAttributes *out,
        irr::io::SAttributeReadWriteOptions *options=0) const;
};

class ComponentFactory : public irr::scene::ISceneNodeAnimatorFactory
{
private:
    irr::scene::ISceneManager* m_SceneMgr;

    struct ComponentType
    {
        ComponentType(irr::scene::ESCENE_NODE_ANIMATOR_TYPE type, const irr::c8* name)
            : Type(type), TypeName(name)
        {}

        irr::scene::ESCENE_NODE_ANIMATOR_TYPE Type;
        irr::core::stringc TypeName;
    };

    static irr::core::array<ComponentType> ms_SupportedComponentTypes;
    static std::map<irr::scene::ESCENE_NODE_ANIMATOR_TYPE, wxXmlDocument> ms_ComponentDefinitions;

public:
    ComponentFactory(irr::scene::ISceneManager* sceneMgr);
    ~ComponentFactory(void);

    irr::scene::ISceneNodeAnimator* createSceneNodeAnimator(irr::scene::ESCENE_NODE_ANIMATOR_TYPE type, 
        irr::scene::ISceneNode *target);
    irr::scene::ISceneNodeAnimator* createSceneNodeAnimator(const irr::c8* type, 
        irr::scene::ISceneNode *target);

    irr::u32 getCreatableSceneNodeAnimatorTypeCount(void) const;

    irr::scene::ESCENE_NODE_ANIMATOR_TYPE getCreateableSceneNodeAnimatorType(irr::u32 idx) const;

    const irr::c8* getCreateableSceneNodeAnimatorTypeName(irr::u32 idx) const;
    const irr::c8* getCreateableSceneNodeAnimatorTypeName(irr::scene::ESCENE_NODE_ANIMATOR_TYPE type) const;

    static irr::scene::ESCENE_NODE_ANIMATOR_TYPE HashComponentName(const wxString& name);
    static void RegisterComponent(const wxString& name, const wxXmlDocument& definition);

private:
    irr::scene::ESCENE_NODE_ANIMATOR_TYPE getTypeFromName(const irr::c8* name) const;
};