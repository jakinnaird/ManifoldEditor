# Irrlicht extension modules

The custom scene nodes used by the editor can be added into
Irrlicht so the output map files can be loaded directly.

Using them in a non-Irrlicht based game engine is not required.

```
    // register the scene node factory
    irr::scene::ISceneNodeFactory* factory = new SceneNodeFactory(m_RenderDevice->getSceneManager());
    m_RenderDevice->getSceneManager()->registerSceneNodeFactory(factory);
    factory->drop();
```
